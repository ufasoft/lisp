#include <el/ext.h>

#include "json-rpc.h"

namespace Ext { namespace Inet { 

String JsonRpcRequest::ToString() const {
	VarValue req;
	if (V20)
		req.Set("jsonrpc", "2.0");
	req.Set("id", Id);
	req.Set("method", Method);
	req.Set("params", Params);
	ptr<MarkupParser> markup = MarkupParser::CreateJsonParser();
	markup->Compact = true;
	ostringstream os;
	markup->Print(os, req);
	return os.str();
}

VarValue JsonResponse::ToVarValue() const {
	VarValue rv;
	if (V20)
		rv.Set("jsonrpc", "2.0");
	rv.Set("id", Id);
	if (Success) {
		rv.Set("result", Result);
	} else {
		VarValue verr;
		verr.Set("code", Code);
		verr.Set("message", JsonMessage);
		if (Data)
			verr.Set("data", Data);
		rv.Set("error", verr);
	}
	return rv;
}

String JsonResponse::ToString() const {
	ptr<MarkupParser> markup = MarkupParser::CreateJsonParser();
	markup->Compact = true;
	ostringstream os;
	markup->Print(os, ToVarValue());
	return os.str();
}

bool JsonRpc::TryAsRequest(const VarValue& v, JsonRpcRequest& req) {
	if (!v.HasKey("method"))
		return false;
	req.Method = v["method"].ToString();
	req.Id = v["id"];
	req.Params = v["params"];
	return true;
}

void JsonRpc::PrepareRequest(JsonRpcRequest *req) {
	req->V20 = V20;
	int id = Interlocked::Increment(m_nextId);
	req->Id = VarValue(id);
	EXT_LOCKED(MtxReqs, m_reqs.insert(make_pair(id, req)));
}

String JsonRpc::Request(RCString method, const vector<VarValue>& params, ptr<JsonRpcRequest> req) {
	if (!req)
		req = new JsonRpcRequest;
	req->Method = method;
	PrepareRequest(req);
	req->Params.SetType(VarType::Array);
	for (int i=0; i<params.size(); ++i)
		req->Params.Set(i, params[i]);
	return req->ToString();
}

String JsonRpc::Request(RCString method, const CJsonNamedParams& params, ptr<JsonRpcRequest> req) {
	if (!req)
		req = new JsonRpcRequest;
	req->Method = method;
	PrepareRequest(req);
	req->Params.SetType(VarType::Map);
	for (CJsonNamedParams::const_iterator it(params.begin()), e(params.end()); it!=e; ++it)
		req->Params.Set(it->first, it->second);
	return req->ToString();
}

String JsonRpc::Notification(RCString method, const vector<VarValue>& params, ptr<JsonRpcRequest> req) {
	if (!req)
		req = new JsonRpcRequest;
	req->Method = method;
	req->Params.SetType(VarType::Array);
	for (int i=0; i<params.size(); ++i)
		req->Params.Set(i, params[i]);
	return req->ToString();
}

JsonResponse JsonRpc::Response(const VarValue& v) {
	JsonResponse r;
	r.V20 = v.HasKey("jsonrpc") && v["jsonrpc"].ToString() == "2.0";
	r.Id = v["id"];
	if (r.Id.type() == VarType::Int) {
		EXT_LOCK (MtxReqs) {
			CRequests::iterator it = m_reqs.find((int)r.Id.ToInt64());
			if (it != m_reqs.end()) {
				r.Request = it->second.first;
				m_reqs.erase(it);
			}
		}
	}

	if (r.Success = (!v.HasKey("error") || v["error"]==VarValue(nullptr))) {
		r.Result = v["result"];
	} else {
		VarValue er = v["error"];
		r.ErrorVal = er;
		if (er.HasKey("code"))
			r.Code = int(er["code"].ToInt64());
		if (er.HasKey("message"))
			r.JsonMessage = er["message"].ToString();
		if (er.HasKey("data"))
			r.Data = er["data"];
	}
	return r;
}

VarValue JsonRpc::ProcessResponse(const VarValue& vjresp) {
	JsonResponse resp = Response(vjresp);
	if (resp.Success)
		return resp.Result;
	JsonRpcExc exc(resp.Code);
	exc.m_message = resp.JsonMessage;
	exc.Data = resp.Data;
	throw exc;
}

VarValue JsonRpc::Call(Stream& stm, RCString method, const vector<VarValue>& params) {
	String s = Request(method, params);
	const char *p = s.c_str();
	stm.WriteBuffer(p, strlen(p));
	stm.Flush();
	return ProcessResponse(MarkupParser::CreateJsonParser()->ParseStream(stm).first);
}

VarValue JsonRpc::Call(Stream& stm, RCString method, const VarValue& arg0) {	
	return Call(stm, method, vector<VarValue>(1, arg0));
}

VarValue JsonRpc::Call(Stream& stm, RCString method, const VarValue& arg0, const VarValue& arg1) {
	vector<VarValue> params(2);
	params[0] = arg0;
	params[1] = arg1;
	return Call(stm, method, params);
}

VarValue JsonRpc::ProcessRequest(const VarValue& v) {
	JsonResponse resp;
	resp.V20 = V20;
	try {
		JsonRpcRequest req;
		if (!TryAsRequest(v, req))
			Throw(E_EXT_JSON_RPC_IsNotRequest);
		if (req.V20)
			V20 = true;
		resp.Result = CallMethod(req.Method, req.Params);
	} catch (RCExc& ex) {
		resp.Success = false;
		resp.JsonMessage = ex.what();
		switch (HRESULT hr = HResultInCatch(ex)) {
		case E_EXT_JSON_RPC_ParseError:		resp.Code = -32700; break;
		case E_EXT_JSON_RPC_IsNotRequest:	resp.Code = -32600; break;
		case E_EXT_JSON_RPC_MethodNotFound:	resp.Code = -32601; break;
		case E_EXT_JSON_RPC_InvalidParams:	resp.Code = -32602; break;
		case E_EXT_JSON_RPC_Internal:		resp.Code = -32603; break;
		default:
			resp.Code = hr;
		}		
	}
	return resp.ToVarValue();
}

void JsonRpc::SendChunk(Stream& stm, const ConstBuf& cbuf) {
	stm.WriteBuf(cbuf);
}

void JsonRpc::ServerLoop(Stream& stm) {
	ptr<MarkupParser> markup = MarkupParser::CreateJsonParser();

	for (pair<VarValue, Blob> pp;  (pp=markup->ParseStream(stm, pp.second)).first;) {
		const VarValue& v = pp.first;
		VarValue vr;
		if (v.type() != VarType::Array)
			vr = ProcessRequest(v);
		else {		// batch
			for (size_t size=v.size(), i=0; i<size; ++i) {
				vr.Set(i, ProcessRequest(v[i]));
			}
		}
		ostringstream os;
		markup->Print(os, vr);
		string s = os.str();
		SendChunk(stm, ConstBuf(s.data(), s.size()));
	}
}




}} // Ext::Inet::


