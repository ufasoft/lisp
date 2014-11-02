#pragma once

#include <el/libext/ext-net.h>

namespace Ext { namespace Inet { namespace FastCGI {

using namespace Ext;

class ServerConnection;
class CgiRequest;
class FastCgiServer;


ENUM_CLASS(RecordType) {
	FCGI_BEGIN_REQUEST = 1,
	FCGI_ABORT_REQUEST = 2,
	FCGI_END_REQUEST = 3,
	FCGI_PARAMS = 4,
	FCGI_STDIN = 5,
	FCGI_STDOUT = 6,
	FCGI_STDERR = 7,
	FCGI_DATA = 8,
	FCGI_GET_VALUES = 9,
	FCGI_GET_VALUES_RESULT = 10,
	FCGI_UNKNOWN_TYPE = 11,
	FCGI_MAXTYPE = FCGI_UNKNOWN_TYPE
} END_ENUM_CLASS(RecordType);

ENUM_CLASS(ProtocolStatus) {
	FCGI_REQUEST_COMPLETE  = 0,
	FCGI_CANT_MPX_CONN   = 1,
	FCGI_OVERLOADED     =  2,
	FCGI_UNKNOWN_ROLE   =  3
} END_ENUM_CLASS(ProtocolStatus);

ostream& operator<<(ostream& os, RecordType typ);

class Record : public Object, public CPersistent {
public:
   	RecordType Type;
   	UInt16 RequestId;
   	mutable Blob Content;

	Record(RecordType type = RecordType::FCGI_UNKNOWN_TYPE)
		:	Type(type)
	{}

   	virtual void BeforeWrite() const {
   	}

	static ptr<Record> ReadFromStream(BinaryReader& rd);
	void Write(BinaryWriter& wr) const override;

   	virtual void ProcessRecord(CgiRequest& cgi, ServerConnection& connection) {}
};

class EndRequestRecord : public Record {
	typedef Record base;
public:
   	UInt32 Status;
   	ProtocolStatus ProtocolStatus;

   	EndRequestRecord()
		:	base(RecordType::FCGI_END_REQUEST)
		,	Status(0)
		,	ProtocolStatus(ProtocolStatus::FCGI_REQUEST_COMPLETE)	
	{
   	}

   	void BeforeWrite() const override {
   		Content.Size = 8;
   		Content[0] = (byte)(Status >> 24);
   		Content[1] = (byte)(Status >> 16);
   		Content[2] = (byte)(Status >> 8);
   		Content[3] = (byte)Status;
   		Content[4] = (byte)ProtocolStatus;	
   	}
 };

class NameValueRecord : public Record {
	typedef Record base;
public:
	NameValueRecord()
		:	base(RecordType::FCGI_PARAMS)
	{}

   	int ReadVariableLen(BinaryReader& rd);
   	void ProcessRecord(CgiRequest& cgi, ServerConnection& connection) override;
 };

class StdinRecord : public Record {
	typedef Record base;
public:
	StdinRecord()
		:	base(RecordType::FCGI_STDIN)
	{}

private:
	void ProcessRecord(CgiRequest& cgi, ServerConnection& connection) override;
};

class BeginRequestRecord : public Record {
	typedef Record base;
public:
	BeginRequestRecord()
		:	base(RecordType::FCGI_BEGIN_REQUEST)
	{}

	bool KeepConn() { return Content[2] & 1; }

	void ProcessRecord(CgiRequest& cgiNull, ServerConnection& connection) override;
};

class AbortRequestRecord : public Record {
	typedef Record base;
public:
	AbortRequestRecord()
		:	base(RecordType::FCGI_ABORT_REQUEST)
	{}

	void ProcessRecord(CgiRequest& cgi, ServerConnection& connection) override;
};

   /*!!!


   public abstract class CgiStream : Stream {
   	internal CgiRequest CgiRequest;

   	public override bool CanSeek {
   		get { return false; }
   	}

   	public override long Length {
   		get { throw new NotImplementedException(); }
   	}

   	public override long Position {
   		get {
   			throw new NotImplementedException();
   		}
   		set {
   			throw new NotImplementedException();
   		}
   	}

   	public override void Flush() {
   	}

   	public override long Seek(long offset, SeekOrigin origin) {
   		throw new NotImplementedException();
   	}

   	public override void SetLength(long value) {
   		throw new NotImplementedException();
   	}

   	public override int Read(byte[] buffer, int offset, int count) {
   		throw new NotImplementedException();
   	}

   	public override void Write(byte[] buffer, int offset, int count) {
   		throw new NotImplementedException();
   	}
   }

   public class InputStream : CgiStream {
   	internal EventWaitHandle Event = new EventWaitHandle(false, EventResetMode.ManualReset);
   	internal byte[] Buffer;
   	internal bool Eof;

   	public override bool CanRead {
   		get { return true; }
   	}

   	public override bool CanWrite {
   		get { return false; }
   	}

   	public override int Read(byte[] buffer, int offset, int count) {
   		if (Buffer == null) {
   			if (Eof)
   				return 0;
   			Event.WaitOne();
   			if (Eof)
   				return 0;
   		}
   		lock (this) {
   			int len = Math.Min(count, Buffer.Length);
   			Array.Copy(Buffer, 0, buffer, offset, len);
   			if (len == Buffer.Length) {
   				Event.Reset();
   				Buffer = null;					
   			}  else {
   				var newBuf = new byte[Buffer.Length - len];
   				Array.Copy(Buffer, len, newBuf, 0, newBuf.Length);
   				Buffer = newBuf;
   			}
   			return len;
   		}			
   	}
   }

   public class OutputStream : CgiStream {
   	internal RecordType RecordType;

   	public override bool CanRead {
   		get { return false; }
   	}

   	public override bool CanWrite {
   		get { return true; }
   	}

   	public override void Write(byte[] buffer, int offset, int count) {
   		Record rec = new Record() { Type = RecordType, RequestId = CgiRequest.RequestId };
   		rec.Content = new byte[count];
   		Array.Copy(buffer, offset, rec.Content, 0, count);
   		CgiRequest.Connection.Send(rec);
   	}
   }*/


class CgiStreambuf : public streambuf {	
public:
	CgiStreambuf(CgiRequest& cgi, RecordType recType);

	~CgiStreambuf() {
		delete m_pMemoryStream;
	}

	void Close();
protected:
	int overflow(int c) override;
	int underflow() override;
	int sync() override;
private:
	mutex m_mtx;
	MemoryStream * volatile m_pMemoryStream;
	volatile bool Eof;
	Blob m_buf;
	ManualResetEvent m_ev;
	CBool m_bSomethingSent;

	CgiRequest& m_cgi;
	RecordType m_recType;

	friend class StdinRecord;
};
   
}}}

template <> struct ptr_traits<Ext::Inet::FastCGI::ServerConnection> {
	typedef Interlocked interlocked_policy;
};

namespace Ext { namespace Inet { namespace FastCGI {

class CgiRequest : public Thread {
	typedef CgiRequest class_type;
public:
	ptr<ServerConnection> Connection;
	NameValueCollection Params;
	UInt16 RequestId;
	CBool KeepConn;

	CgiStreambuf InputStream, OutputStream, ErrorStream;

	istream In;
	ostream Out, Error;

	CgiRequest(ServerConnection& conn)
		:	Connection(&conn)
		,	InputStream(_self, RecordType::FCGI_STDIN)
		,	OutputStream(_self, RecordType::FCGI_STDOUT)
		,	ErrorStream(_self, RecordType::FCGI_STDERR)
		,	In(&InputStream)
		,	Out(&OutputStream)
		,	Error(&ErrorStream)	
	{
//		StackSize = UCFG_THREAD_STACK_SIZE;
	}		

	String get_RequestUri() {
		String r = Params.Get("REQUEST_URI");
		if (r == nullptr)
			Throw(E_FAIL);
		return r;
	}
	DEFPROP_GET(String, RequestUri);

	String get_PathInfo() {
		String r = Params.Get("PATH_INFO");
		if (r == nullptr)
			Throw(E_FAIL);
		return r;
	}
	DEFPROP_GET(String, PathInfo);

	String get_QueryString() {
		return Params.Get("QUERY_STRING");
	}
	DEFPROP_GET(String, QueryString);

	String get_Accept() {
		return Params.Get("HTTP_ACCEPT");
	}
	DEFPROP_GET(String, Accept);

	String get_UserAgent() {
		return Params.Get("HTTP_USER_AGENT");
	}
	DEFPROP_GET(String, UserAgent);

	IPAddress get_UserHostAddress() {
		return IPAddress::Parse(Params.Get("REMOTE_ADDR"));
	}
	DEFPROP_GET(IPAddress, UserHostAddress);

	bool CheckForDoSAttack();
protected:
	void Execute() override;
	virtual void OnDoSAttack();
private:
};

class ServerConnection : public Thread {
public:
   	FastCgiServer& Server;
	mutex MtxRequests;
	unordered_map<UInt16, ptr<CgiRequest>> Requests;

	Socket m_sock;
	NetworkStream m_stm;
	mutex MtxWriter;
	mutex MtxReader;
   	BinaryReader Reader;
   	BinaryWriter Writer;

	ServerConnection(FastCgiServer& server);

   	void Send(const Record& rec);
protected:
	void Execute() override;
};

class ClientHistory {
public:
	static const int MAX_HISTORY_SIZE = 1024;

	deque<DateTime> Calls;
};

class FastCgiServer : public SocketThread {
	typedef SocketThread base;
public:
	IPEndPoint ListeningEndpoint;
	
	CInt<int> PerSecondLimit, PerMinuteLimit, PerHourLimit;

	FastCgiServer(thread_group& tr)
		:	base(&tr)
		,	ListeningEndpoint(IPAddress::Loopback, 900)
	{}
protected:
	Socket m_sock;

	mutex m_mtxHistory;
	LruMap<IPAddress, ClientHistory> m_ip2history;
	
	void BeforeStart() override;
	void Execute() override;	
   	virtual void ProcessCgiRequest(CgiRequest& cgi);

	friend class CgiRequest;
};



}}} // Ext::Inet::FastCGI::
