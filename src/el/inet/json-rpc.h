#pragma once

namespace Ext { namespace Inet {
using namespace Ext;

typedef unordered_map<String, VarValue> CJsonNamedParams;

class JsonRpcExc : public Exception {
	typedef Exception base;
public:
	int Code;
	VarValue Data;

	JsonRpcExc(int code)
		:	base(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_JSON_RPC, UInt16(code)))
		,	Code(code)
	{}

	~JsonRpcExc() noexcept {}
};

class JsonRpcRequest : public Object {
public:
	typedef Interlocked interlocked_policy;

	VarValue Id;
	String Method;
	VarValue Params;
	CBool V20;

	String ToString() const;
};

class JsonResponse {
public:
	ptr<JsonRpcRequest> Request;

	VarValue Id;
	VarValue Result;
	VarValue ErrorVal;
	VarValue Data;

	String JsonMessage;
	int Code;
	bool Success;
	bool V20;

	JsonResponse()
		:	Success(true)
		,	V20(false)
		,	Code(0)
		,	Id(nullptr)
	{}

	VarValue ToVarValue() const;
	String ToString() const;
};

class JsonRpc {
public:
	CBool V20;

	JsonRpc()
		:	m_nextId(1)
	{}

	static bool TryAsRequest(const VarValue& v, JsonRpcRequest& req);
	String Request(RCString method, const vector<VarValue>& params = vector<VarValue>(), ptr<JsonRpcRequest> req = nullptr);
	String Request(RCString method, const CJsonNamedParams& params, ptr<JsonRpcRequest> req = nullptr);
	VarValue ProcessResponse(const VarValue& vjresp);
	VarValue Call(Stream& stm, RCString method, const vector<VarValue>& params = vector<VarValue>());
	VarValue Call(Stream& stm, RCString method, const VarValue& arg0);
	VarValue Call(Stream& stm, RCString method, const VarValue& arg0, const VarValue& arg1);

	String Notification(RCString method, const vector<VarValue>& params = vector<VarValue>(), ptr<JsonRpcRequest> req = nullptr);
	JsonResponse Response(const VarValue& v);
	void ServerLoop(Stream& stm);
	
protected:
	virtual void SendChunk(Stream& stm, const ConstBuf& cbuf);
	virtual VarValue CallMethod(RCString name, const VarValue& params) { Throw(E_EXT_JSON_RPC_MethodNotFound); }
private:
	std::mutex MtxReqs;

	typedef LruMap<int, ptr<JsonRpcRequest>> CRequests;
	CRequests m_reqs;

	volatile Int32 m_nextId;

	void PrepareRequest(JsonRpcRequest *req);
	VarValue ProcessRequest(const VarValue& v);
};






}} // Ext::Inet::

