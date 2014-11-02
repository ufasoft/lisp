#include <el/ext.h>

#include "fastcgi.h"


namespace Ext { namespace Inet { namespace FastCGI {

ostream& operator<<(ostream& os, RecordType typ) {
	const char *s;
	switch (typ) {
	case RecordType::FCGI_BEGIN_REQUEST: s = "FCGI_BEGIN_REQUEST"; break;
	case RecordType::FCGI_ABORT_REQUEST: s = "FCGI_ABORT_REQUEST"; break;
	case RecordType::FCGI_END_REQUEST:	s = "FCGI_END_REQUEST"; break;
	case RecordType::FCGI_PARAMS:		s = "FCGI_PARAMS"; break;
	case RecordType::FCGI_STDIN :		s = "FCGI_STDIN "; break;
	case RecordType::FCGI_STDOUT:		s = "FCGI_STDOUT"; break;
	case RecordType::FCGI_STDERR:		s = "FCGI_STDERR"; break;
	case RecordType::FCGI_DATA:			s = "FCGI_DATA"; break;
	case RecordType::FCGI_GET_VALUES: s = "FCGI_GET_VALUES"; break;
	case RecordType::FCGI_GET_VALUES_RESULT: s = "FCGI_GET_VALUES_RESULT"; break;
	default:
		os << "Unknown Record Type: " << (int)typ;
		return os;
	}
	os << s;
	return os;
}

ptr<Record> Record::ReadFromStream(BinaryReader& rd) {
	ptr<Record> r;
	switch (rd.BaseStream.ReadByte()) {
	case -1: return nullptr;
	case 1:
		{
			RecordType type = (RecordType)rd.BaseStream.ReadByte();

			TRC(4, type);

			switch (type) {
			case RecordType::FCGI_PARAMS:			r = new NameValueRecord();		break;
			case RecordType::FCGI_STDIN:			r = new StdinRecord();			break;
			case RecordType::FCGI_BEGIN_REQUEST:	r = new BeginRequestRecord();	break;
			case RecordType::FCGI_ABORT_REQUEST:	r = new AbortRequestRecord();	break;
			default:								r = new Record(type);			break;
			}
			r->RequestId = ntohs(rd.ReadUInt16());

			int len = ntohs(rd.ReadUInt16());
			int padLen = rd.ReadByte();
			rd.ReadByte();
			if (len != 0) {
				r->Content.Size = len;
				rd.BaseStream.ReadBuffer(r->Content.data(), len);
			}
			if (padLen != 0) {
				void *pad = alloca(padLen);
				rd.BaseStream.ReadBuffer(pad, padLen);
			}
		}
		break;
	default:
		Throw(E_EXT_Protocol_Violation);
	}
	return r;
}

void Record::Write(BinaryWriter& wr) const {
	TRC(4, "Type: " << Type);

	MemoryStream ms;
	BinaryWriter(ms) << byte(1) << byte(Type) << htons(RequestId) << htons((UInt16)Content.Size) << byte(0) << byte(0);
	ms.WriteBuf(Content);
	wr.BaseStream.WriteBuf(ms);
}


int NameValueRecord::ReadVariableLen(BinaryReader& rd) {
   	int len = rd.ReadByte();
   	if (0 != (len & 0x80)) {
   		len = (len & 0x7F) << 24 | (rd.ReadByte() << 16);
   		len |= (rd.ReadByte() << 8);
   		len |= rd.ReadByte();
   	}
   	return len;
}

void NameValueRecord::ProcessRecord(CgiRequest& cgi, ServerConnection& connection) {
   	if (Content.Size == 0) {
   		cgi.Start();
   		return;
   	}

	CMemReadStream stm(Content);
	BinaryReader rd(stm);
   	while (stm.Position != stm.Length) {
   		int nameLen = ReadVariableLen(rd);
   		int valLen = ReadVariableLen(rd);
   		String name, value;
   		if (nameLen != 0)
   			name = Encoding::UTF8.GetChars(rd.ReadBytes(nameLen));
   		if (valLen != 0)
   			value = Encoding::UTF8.GetChars(rd.ReadBytes(valLen));
   		cgi.Params.Set(name, value);
   	}
}

void BeginRequestRecord::ProcessRecord(CgiRequest& cgiNull, ServerConnection& connection) {
	if (Content.Size != 8)
		Throw(E_EXT_Protocol_Violation);
	ptr<CgiRequest> cgi = new CgiRequest(connection);
	cgi->RequestId = RequestId;
	cgi->KeepConn = KeepConn();

	EXT_LOCK (connection.MtxRequests) {
		connection.Requests[RequestId] = cgi;
	}
}

void StdinRecord::ProcessRecord(CgiRequest& cgi, ServerConnection& connection) {
   	CgiStreambuf& istm = cgi.InputStream;
   	EXT_LOCK (istm.m_mtx) {
   		if (Content.Size == 0) {
   			istm.Eof = true;
   		} else {
			if (!istm.m_pMemoryStream)
				istm.m_pMemoryStream = new MemoryStream;
			istm.m_pMemoryStream->WriteBuf(Content);
   		}
   	}
   	istm.m_ev.Set();
}

CgiStreambuf::CgiStreambuf(CgiRequest& cgi, RecordType recType)
	:	m_cgi(cgi)
	,	m_recType(recType)
	,	Eof(false)
	,	m_pMemoryStream(0)
	,	m_buf(0, 4096)
{
	setp((char*)m_buf.data(), (char*)m_buf.data()+m_buf.Size);
}

void CgiStreambuf::Close() {
	if (m_bSomethingSent) {
		Record rec(m_recType);
		rec.RequestId = m_cgi.RequestId;
		m_cgi.Connection->Send(rec);
	}
}

int CgiStreambuf::sync() {
	Record rec(m_recType);
	rec.RequestId = m_cgi.RequestId;
	if ((rec.Content = Blob(pbase(), pptr()-pbase())).Size != 0) {
		setp(pbase(), epptr());
		m_cgi.Connection->Send(rec);
		m_bSomethingSent = true;
	}
	return 0;
}

int CgiStreambuf::overflow(int c) {
	Record rec(m_recType);
	rec.RequestId = m_cgi.RequestId;
	rec.Content = Blob(pbase(), pptr()-pbase())+ConstBuf(&c, 1);
	setp(pbase(), epptr());
	m_cgi.Connection->Send(rec);
	m_bSomethingSent = true;
	return 1;
}

int CgiStreambuf::underflow() {
	if (!m_pMemoryStream) {
		if (Eof)
			return EOF;
		m_ev.Lock();
	}
	EXT_LOCK (m_mtx) {
		m_buf = m_pMemoryStream->Blob;
		setg((char*)m_buf.data(), (char*)m_buf.data(), (char*)m_buf.data()+m_buf.Size);
		m_ev.Reset();
		delete exchange(m_pMemoryStream, nullptr);
	}
	return *gptr();
}

void AbortRequestRecord::ProcessRecord(CgiRequest& cgi, ServerConnection& connection) {
	cgi.interrupt();
}

void CgiRequest::Execute() {
	Name = "CgiRequest";

	EndRequestRecord rec;
	rec.RequestId = RequestId;
	try {
		Connection->Server.ProcessCgiRequest(_self);
	} catch (RCExc ex) {
		Error << "Error: " << ex.what();
		rec.Status = 500;  //!!!?
	}
	try {
		Out.flush();
		Error.flush();
		OutputStream.Close();
		ErrorStream.Close();
	
		Connection->Send(rec);
		
		if (!KeepConn)
			Connection->m_stm.m_sock.Shutdown();
	} catch (RCExc ex) {
		TRC(1, ex.what());
	}
	EXT_LOCK (Connection->MtxRequests) {
		Connection->Requests.erase(RequestId);
	}
}

bool CgiRequest::CheckForDoSAttack() {
	FastCgiServer& server = Connection->Server;
	if (server.PerSecondLimit) {
		DateTime now = DateTime::UtcNow();
		IPAddress ip = UserHostAddress;
		EXT_LOCK (server.m_mtxHistory) {
			auto it = server.m_ip2history.find(ip);
			if (it != server.m_ip2history.end()) {
				ClientHistory& history = it->second.first;
				DateTime secondAgo = now - TimeSpan::FromSeconds(1),
						minuteAgo = now - TimeSpan::FromMinutes(1),
						hourAgo = now - TimeSpan::FromHours(1);
				int sec = 0, minut = 0, hour = 0;
				EXT_FOR (const DateTime& dt, history.Calls) {
					sec += int(dt > secondAgo);
					minut += int(dt > minuteAgo);
					hour += int(dt > hourAgo);
				}
				if (history.Calls.size() >= ClientHistory::MAX_HISTORY_SIZE)
					history.Calls.erase(history.Calls.begin());
				history.Calls.push_back(now);
				if (sec > server.PerSecondLimit || minut > server.PerMinuteLimit || hour > server.PerHourLimit) {
					OnDoSAttack();
					return false;
				}
			} else {
				ClientHistory history;
				history.Calls.push_back(now);
				server.m_ip2history.insert(make_pair(ip, history));
			}		
		}
	}
	return true;
}

void CgiRequest::OnDoSAttack() {
	TRC(1, "DoS from " << UserHostAddress);

	Out << "Content-type: text/plain\r\n"
		"Status: 429 Too Many Requests\r\n"		
		"\r\n";
   	Out << "Too Many Requests from your IP address\n"
		"Please, try later" << endl;
}

ServerConnection::ServerConnection(FastCgiServer& server)
	:	Server(server)
	,	m_stm(m_sock)
	,	Reader(m_stm)
	,	Writer(m_stm)

{
}

void ServerConnection::Send(const Record& rec) {
   	rec.BeforeWrite();
	EXT_LOCK (MtxWriter) {
		Writer << rec;
		Writer.BaseStream.Flush();
	}
}

void ServerConnection::Execute() {
	Name = "ServerConnection";

   	try {
		while (true) {
			ptr<Record> rec;
			EXT_LOCK (MtxReader) {
				rec = Record::ReadFromStream(Reader);
			}
			if (!rec)
				break;
			ptr<CgiRequest> cgi;
			EXT_LOCK (MtxRequests) {
				Lookup(Requests, rec->RequestId, cgi);
			}
   			if (cgi || rec->Type == RecordType::FCGI_BEGIN_REQUEST)
   				rec->ProcessRecord(*cgi, _self);
   		}
   	} catch (RCExc ex) {
   		TRC(2, ex.what());
   	}
	EXT_LOCK (MtxRequests) {
		Requests.clear();						// break cyclic refs
	}
}

void FastCgiServer::BeforeStart() {
	m_sock.Open();
	m_sock.Bind(ListeningEndpoint);
	TRC(2, "Listening on FastCGI endpoint  " << ListeningEndpoint);
}

void FastCgiServer::Execute() {
	Name = "FastCgiServer";

	m_sock.Listen();

	SocketKeeper sk(_self, m_sock);
	Socket s;
	IPEndPoint ep;
	try {
		DBG_LOCAL_IGNORE_WIN32(WSAEINTR);

		while (m_sock.Accept(s, ep)) {
			TRC(2, "FastCGI connect from " << ep);

			ptr<ServerConnection> conn = new ServerConnection(_self);
			conn->m_sock = move(s);
			conn->Start();
   		}
	} catch (RCExc) {
	}
}

void FastCgiServer::ProcessCgiRequest(CgiRequest& cgi) {
	cgi.Out << "Content-type: text/plain\r\n\r\n";
   	cgi.Out << "FastCGI request" << endl;
}


}}} // Ext::Inet::FastCGI::



