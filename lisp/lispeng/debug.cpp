/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

#if UCFG_LISP_DEBUG

void CLispEng::PrintList(ostream& os, CP form) {
	Print(os, Car(form));
	CSPtr cdr = Cdr(form);
	switch (Type(cdr))
	{
		/*!!!  case TS_NIL:
		break;*/
	case TS_CONS:
		if (cdr) {			//!!! 
			if (cdr == V_U)
				os << "<UNDEF>";
			else
				os << " ";
			PrintList(os, cdr);
		}
		break;
	default:
		os << " . ";
		Print(os, cdr);
	}
}


const char *g_arOpcode[256] = {
	"NIL",		"PUSH-NIL",	"T",			"CONST",	"LOAD",		"LOADI",		"LOADC",		"LOADV",
	"LOADIC", "STORE",		"STOREI", "STOREC", "STOREV", "STOREIC",	"GETVALUE", "SETVALUE",
	"BIND",		"UNBIND1",	"UNBIND", "PROGV",	"PUSH",		"POP",			"SKIP",			"SKIPI",
	"SKIPSP",	"SKIP&RET",	"SKIP&RETGF",	"JMP", "JMPIF",	"JMPIFNOT",	"JMPIF1",		"JMPIFNOT1",

	"JMPIFATOM", "JMPIFCONSP", "JMPIFEQ", "JMPIFNOTEQ", "JMPIFEQTO", "JMPIFNOTEQTO", "JMPHASH", "JMPHASHV",
	"JSR",		"JMPTAIL",	"VENV",		"MAKE-VECTOR1&PUSH", "COPY-CLOSURE", "CALL",	"CALL0", "CALL1",
	"CALL2",	"CALLS1",		"CALLS2",	"CALLSR",	"CALLC",	"CALLCKEY",	"FUNCALL",	"APPLY",
	"PUSH-UNBOUND",	"UNLIST",	"UNLIST",	"JMPIFBOUNDP"	, "BOUNDP",	"UNBOUND->NIL",	"VALUES0",	"VALUES1",

	"STACK-TO-MV",	"MV-TO-STACK",	"NV-TO-STACK",	"MV-TO-LIST",	"LIST-TO-MV",	"MVCALLP",	"MVCALL",	"BLOCK-OPEN",
	"BLOCK-CLOSE",	"RETURN-FROM",	"RETURN-FROM-I",	"TAGBODY-OPEN",	"TAGBODY-CLOSE-NIL",	"TAGBODY-CLOSE",	"GO", "GO-I",
	"CATCH-OPEN",	  "CATCH-CLOSE",	"THROW",	"UNWIND-PROTECT-OPEN", "UNWIND-PROTECT-NORMAL-EXIT",	"UNWIND-PROTECT-CLOSE", "UNWIND-PROTECT-CLEANUP", "HANDLER-OPEN",
	"HANDLER-BEGIN&PUSH", "NOT",		"EQ",			"CAR",	"CDR",	"CONS", "SYMBOL-FUNCTION", "SVREF",

	"SVSET", "LIST", "LIST*", "NIL&PUSH", "T&PUSH", "CONST&PUSH", "LOAD&PUSH", "LOADI&PUSH",
	"LOADC&PUSH", "LOADV&PUSH", "POP&STORE", "GETVALUE&PUSH", "JSR&PUSH", "COPY-CLOSURE&PUSH", "CALL&PUSH", "CALL1&PUSH",
	"CALL2&PUSH", "CALLS1&PUSH", "CALLS2&PUSH", "CALLSR&PUSH", "CALLC&PUSH", "CALLCKEY&PUSH", "FUNCALL&PUSH", "APPLY&PUSH",
	"CAR&PUSH", "CDR&PUSH", "CONS&PUSH", "LIST&PUSH", "LIST*&PUSH", "NIL&STORE", "T&STORE", "LOAD&STOREC",

	"CALLS1&STORE", "CALLS2&STORE", "CALLSR&STORE", "LOAD&CDR&STORE", "LOAD&CONS&STORE", "LOAD&INC&STORE", "LOAD&DEC&STORE", "LOAD&CAR&STORE",
	"CALL1&JMPIF", "CALL1&JMPIFNOT", "CALL2&JMPIF", "CALL2&JMPIFNOT", "CALLS1&JMPIF", "CALLS1&JMPIFNOT", "CALLS2&JMPIF", "CALLS2&JMPIFNOT",
	"CALLSR&JMPIF", "CALLSR&JMPIFNOT", "LOAD&JMPIF", "LOAD&JMPIFNOT", "LOAD&CAR&PUSH", "LOAD&CDR&PUSH", "LOAD&INC&PUSH", "LOAD&DEC&PUSH",
	"CONST&SYMBOL-FUNCTION", "CONST&SYMBOL-FUNCTION&PUSH", "CONST&SYMBOL-FUNCTION&STORE", "APPLY&SKIP&RET", "FUNCALL&SKIP&RETGF", "LOAD0", "LOAD1", "LOAD2",

	"LOAD3",			"LOAD4",			"LOAD5",			"LOAD6",			"LOAD7",	"LOAD8",	"LOAD9",	"LOAD10",
	"LOAD11",			"LOAD12",			"LOAD13",			"LOAD14",			"LOAD_PUSH0", "LOAD_PUSH1", "LOAD_PUSH2", "LOAD_PUSH3",
	"LOAD_PUSH4", "LOAD_PUSH5", "LOAD_PUSH6", "LOAD_PUSH7",	"LOAD_PUSH8", "LOAD_PUSH9", "LOAD_PUSH10", "LOAD_PUSH11",
	"LOAD_PUSH12", "LOAD_PUSH13", "LOAD_PUSH14", "LOAD_PUSH15", "LOAD_PUSH16", "LOAD_PUSH17", "LOAD_PUSH18", "LOAD_PUSH19",

	"LOAD_PUSH20", "LOAD_PUSH21",      "LOAD_PUSH22",      "LOAD_PUSH23",
	"LOAD_PUSH24",      "CONST0",           "CONST1",           "CONST2",
	"CONST3",           "CONST4",           "CONST5",           "CONST6",
	"CONST7",           "CONST8",           "CONST9",           "CONST10",

	"CONST11",          "CONST12",          "CONST13",          "CONST14",
	"CONST15",          "CONST16",          "CONST17",          "CONST18",
	"CONST19",          "CONST20",          "CONST_PUSH0",      "CONST_PUSH1",
	"CONST_PUSH2",      "CONST_PUSH3",      "CONST_PUSH4",      "CONST_PUSH5",

	"CONST_PUSH6",      "CONST_PUSH7",      "CONST_PUSH8",      "CONST_PUSH9",
	"CONST_PUSH10",     "CONST_PUSH11",     "CONST_PUSH12",     "CONST_PUSH13",
	"CONST_PUSH14",     "CONST_PUSH15",     "CONST_PUSH16",     "CONST_PUSH17",
	"CONST_PUSH18",     "CONST_PUSH19",     "CONST_PUSH20",     "CONST_PUSH21",

	"CONST_PUSH22",     "CONST_PUSH23",     "CONST_PUSH24",     "CONST_PUSH25",
	"CONST_PUSH26",     "CONST_PUSH27",     "CONST_PUSH28",     "CONST_PUSH29",
	"STORE0",           "STORE1",           "STORE2",           "STORE3",
	"STORE4",           "STORE5",           "STORE6",           "STORE7"
};

CClosure *g_pClos;

void CLispEng::ReadPrintLabel(ostream& os) {
	LONG s = ReadS();
	byte *p = CurVMContext->m_pb+s;
	int off = g_pClos->Flags & FLAG_KEY ? CCV_START_KEY : CCV_START_NONKEY;
	os << "off=" << s << " L" << DWORD(p-(byte*)AsArray(g_pClos->CodeVec)->m_pData-off);
}

void CLispEng::PrintValues(ostream& os) {
	for (int i=0; i<m_cVal; i++)
		Print(m_arVal[i]);
}

void CLispEng::Print(ostream& os, CP form) {
	form &= ~FLAG_Mark;
	//!!!	os << "<" << (void*)form << ">";
	
	os << String(' ', m_printIndent*2);

	switch (Type(form))
	{
		/*!!!  case TS_NIL:
		os << "NIL";
		break;*/
	case TS_CHARACTER:
		{
			os << "#\\";
			wchar_t ch = AsChar(form);
			switch (ch)
			{
			case '\0': os << "NULL"; break;
			case '\a': os << "BELL"; break;
			case '\b': os << "BS"; break;
			case '\t': os << "TAB"; break;
			case '\n': os << "NL"; break;
			case '\r': os << "CR"; break;
			case '\f': os << "FF"; break;
			case '\x1B': os << "ESC"; break;
			case '\x7F': os << "RUBOUT"; break;
			default: os << (char)ch; break; //!!!
			}
		}
		break;
	case TS_FIXNUM:
		os << AsNumber(form);
		break;
	case TS_FRAME_PTR:
		os << "#<FRAME " << (DWORD)AsIndex(form) << ">";
		break;
	case TS_FRAMEINFO:
		os << "#<FRAMEINFO>";
		break;
	case TS_BIGNUM:
		os << ToBigInteger(form);
		break;
	case TS_FLONUM:
		os << AsFloatVal(form);
		break;
	case TS_SYMBOL:
		{
			//!!!			Print(get_Special(L_S_PACKAGE));//!!!D
			CSymbolValue *sv = ToSymbol(form);
			if (sv->HomePackage == m_packKeyword)
				os << ':';
			else if (!sv->HomePackage)
				os << "#:";
			/*!!!      else if (!ToPackage(get_Special(L_S_PACKAGE))->IsPresent(form))
			{
			CPackage *pack = ToPackage(sv->HomePackage);
			os << pack->m_name << ':';
			if (pack->m_mapExternalSym.find(sv) == pack->m_mapExternalSym.end())
			os << ':';
			}*/
#if UCFG_LISP_SPLIT_SYM
			os << SymNameByIdx(AsIndex(form));
#else
			os << sv->m_s;
#endif
		}
		break;
	case TS_PATHNAME:
		os << "#P\"" << AsPathname(form)->ToString() << "\"";
		break;
	case TS_RATIO:
		Print(os, AsRatio(form)->m_numerator);
		os << "/";
		Print(os, AsRatio(form)->m_denominator);
		break;//!!!
	case TS_MACRO:
		os << "#<MACRO>";
		break;
	case TS_COMPLEX:
		os << "#C()"; //!!!
		break;
	case TS_CONS:
		switch (form)
		{
		case 0:
			os << "NIL";
			break;
		case V_U:
			os << "#<UNBOUND>";
			break;
		default:
			CSPtr car = Car(form),
				cdr = Cdr(form);
			if (car == S(L_QUOTE)) {
				if (ConsP(cdr)) {
					os << "\'";
					Print(os, Car(cdr));
					break;
				}
			}
			if (car == S(L_BACKQUOTE)) {
				os << "`";
				Print(os, Car(cdr));
			} else if (car == S(L_UNQUOTE)) {
				os << ",";
				Print(os, Car(cdr));
			} else if (car == S(L_SPLICE)) {
				os << ",@";
				Print(os, Car(cdr));
			} else {
				os << "(";
				PrintList(os, form);
				os << ")";
			}
		}
		break;
	case TS_STRUCT:
		os << "#<STRUCT " << (DWORD)AsIndex(form) << ">";
		break;
	case TS_STREAM:
		os << "#<STREAM " << (DWORD)AsIndex(form) << ">";
		break;
	case TS_WEAKPOINTER:
		os << "<#WEAKPOINTER ";
		Print(os, ToWeakPointer(form)->m_p);
		os << ">";
		break;
	case TS_SUBR:
	case TS_INTFUNC:
	case TS_CCLOSURE:
		{
			os << "#\'";
			Push(m_r);
			pair<CP, CP> pp = GetFunctionName(form);
			Print(os, pp.second);
			m_r = Pop();
		}
		break;
	case TS_PACKAGE:
		os << "#<PACKAGE " << ToPackage(form)->m_name << ">";
		break;
	case TS_READTABLE:
		os << "#<READTABLE>";
		break;
	case TS_HASHTABLE:
		{
			os << "#S(HASH-TABLE  <fun>";
			CHashMap& m = *Lisp().ToHashTable(form)->m_pMap;
			for (CHashMap::iterator i=m.begin(); i!=m.end(); ++i) {
				os << "(";
				Print(os, i->first);
				os << " . ";
				Print(os, i->second);
				os << ")";
			}
			os << ">";
		}
		break;
	case TS_ARRAY:
		if (StringP(form))
			os << "\"" << AsString(form) << "\"";
		else if (VectorP(form)) {
			os << "#(";
			CArrayValue *av = ToArray(form);
			for (int i=0; i<av->GetVectorLength(); i++) {
				if (i)
					os << " ";
				Print(os, av->GetElement(i));
			}
			os << ")";
		} else {
			CArrayValue *av = ToArray(form);
			os << "<ARRAY " << (DWORD)av->TotalSize() << ">";

			//!!!
			/*!!!        os << "#(";
			for (int i=0; i<vv->m_count-1; i++)
			{
			Print(vv->m_pData[i]);
			os << " ";
			}
			os << ")";*/
		}
		break;
	case TS_OBJECT:
		{
			os << "#<OBJECT ";
			//!!!      Print(ToObject(form)->m_class);
			os << " ";
			//!!!      Print(ToObject(form)->m_slots);
			os << ">";
		}
		break;
	case TS_SYMBOLMACRO:
		{
			os << "#<SYMBOL-MACRO ";
			CSymbolMacro *sm = ToSymbolMacro(form);
			Print(os, sm->m_macro);
			os << ">";
		}
		break;
#if UCFG_LISP_FFI
	case TS_FF_PTR:
		os << "#<POINTER " << ToPointer(form) << ">";
		break;
#endif

	default:
		os << "#<Bad Form>";
		//!!!    Ext::Throw(E_FAIL);
	}
}

void CLispEng::Disassemble(ostream& os, CP p) {
	CClosure *c = ToCClosure(p);
	g_pClos = c;
	os << "\n";
	Print(c->NameOrClassVersion);
	os << "\nConsts:";
	if (AsArray(p)->DataLength)
		for (int i=0; i<AsArray(p)->DataLength; i++) {
			os << "\n  CONST[" << i << "] = ";
			Print(c->Consts[i]);
		}
	os << "\n\n";
	int off = c->Flags & FLAG_KEY ? CCV_START_KEY : CCV_START_NONKEY;
	byte *prevPb = CurVMContext->m_pb;
	for (ssize_t i = off; i<AsArray(c->CodeVec)->GetVectorLength();) {
		byte *pb = (byte*)AsArray(c->CodeVec)->m_pData; 
		os << DWORD(i-off) << "\t";
		byte b = pb[i++];
		os << Convert::ToString(b, "X2") << "  " << g_arOpcode[b] << "\t";
		VM_CONTEXT;
		CurVMContext->m_pb = pb+i;
		switch (b) {
		case COD_CALLS1:
		case COD_CALLS2:
		case COD_CALLS1_PUSH:
		case COD_CALLS2_PUSH:
			os << int(ReadB());
			break;
		case COD_CALLSR:
		case COD_CALLSR_PUSH:
			os << ReadU() << " ";
			os << int(ReadB());
			break;
		case COD_CALLS1_JMPIF:
		case COD_CALLS1_JMPIFNOT:
		case COD_CALLS2_JMPIF:
		case COD_CALLS2_JMPIFNOT:
			os << int(ReadB()) << " ";
			ReadPrintLabel(os);
			break;
		case COD_BLOCKOPEN:
		case COD_LOAD_JMPIF:
		case COD_LOAD_JMPIFNOT:
		case COD_CALL1_JMPIF:
		case COD_CALL1_JMPIFNOT:
		case COD_CALL2_JMPIF:
		case COD_CALL2_JMPIFNOT:
		case COD_JMPIFBOUNDP:
		case COD_JMPIFEQTO:
		case COD_JMPIFNOTEQTO:
			os << ReadU() << " ";
			ReadPrintLabel(os);
			break;
		case COD_CALLSR_JMPIF:
		case COD_CALLSR_JMPIFNOT:
			os << ReadU() << " " << ReadU() << " ";
			ReadPrintLabel(os);
			break;
		case COD_CATCHOPEN:
		case COD_UWPOPEN:
		case COD_JSR:
		case COD_JMP:
		case COD_JMPIF:
		case COD_JMPIF1:
		case COD_JMPIFNOT:
		case COD_JMPIFNOT1:
		case COD_JMPIFATOM:
		case COD_JMPIFCONSP:
		case COD_JMPIFEQ:
		case COD_JMPIFNOTEQ:
			ReadPrintLabel(os);
			break;
		case COD_RETURNFROM:
		case COD_JMPHASH:
		case COD_JMPHASHV:
		case COD_APPLY:
		case COD_CALL0:
		case COD_CALL1:
		case COD_CALL2:
		case COD_CALL1_PUSH:
		case COD_CALL2_PUSH:
		case COD_CONST_PUSH:
		case COD_POP_STORE:
		case COD_LOAD:
		case COD_LOAD_PUSH:
		case COD_SETVALUE:
		case COD_LOAD_CAR_PUSH:
		case COD_LOAD_CDR_STORE:
		case COD_LOAD_INC_PUSH:
		case COD_LOAD_DEC_PUSH:
		case COD_LOAD_INC_STORE:
		case COD_LOAD_DEC_STORE:
		case COD_GETVALUE:
		case COD_GETVALUE_PUSH:
		case COD_PUSH_UNBOUND:
		case COD_PUSH_NIL:
		case COD_BIND:
		case COD_UNBIND:
		case COD_UNBOUND_NIL:
		case COD_NVTOSTACK:
		case COD_SKIP:
		case COD_SKIP_RET:
		case COD_SKIP_RETGF:
		case COD_FUNCALL:
		case COD_FUNCALL_PUSH:
		case COD_MAKEVECTOR1_PUSH:
		case COD_CONST:
		case COD_CONST_SYMBOLFUNCTION:
		case COD_CONST_SYMBOLFUNCTION_PUSH:
		case COD_LIST_PUSH:
		case COD_LISTSTERN_PUSH:
		case COD_STACKTOMV:
		case COD_BOUNDP:
		case COD_HANDLEROPEN:
			os << ReadU();
			break;
		case COD_CALL:
		case COD_CALL_PUSH:
		case COD_LOADC:
		case COD_LOADV:
		case COD_LOADV_PUSH:
		case COD_STOREC:
		case COD_STOREV:
		case COD_COPYCLOSURE:
		case COD_COPYCLOSURE_PUSH:
		case COD_FUNCALL_SKIP_RETGF:
		case COD_APPLY_SKIP_RET:
		case COD_UNLISTSTERN:
		case COD_UNLIST:
		case COD_SKIPSP:
			os << ReadU() << " ";
			os << ReadU();
			break;
		case COD_RETURNFROMI:
		case COD_LOADI:
		case COD_LOADI_PUSH:
		case COD_CALLSR_STORE:
			os << ReadU() << " ";
			os << ReadU() << " ";
			os << ReadU();
			break;
		case COD_STOREIC:
			os << ReadU() << " ";
			os << ReadU() << " ";
			os << ReadU() << " ";
			os << ReadU();
			break;
		}
		i = CurVMContext->m_pb-pb;

		os << "\n";
	}
	CurVMContext->m_pb = prevPb;
	os << endl;
}

void CLispEng::PrintFrames() {
	for (CP *p = m_pStack; p != m_pStackTop; p++) {
		if (Type(*p) == TS_FRAMEINFO) {
			if (AsFrameType(*p) == FT_EVAL) {
				cerr << "\n\nEVAL frame for form ";
				Print(p[1]);
			} else if (AsFrameType(*p) == FT_APPLY) {
				cerr << "\n\nAPPLY frame for form ";
				Print(p[1]);
			}
		}
	}
}

#else

void CLispEng::PrintFrames() {} 
void CLispEng::Print(ostream& os, CP form) {}
void CLispEng::PrintValues(ostream& os) {}
void CLispEng::Disassemble(ostream& os, CP p) {}

#endif // UCFG_LISP_DEBUG

void CVerifier::Verify(CP p) {
	CTypeSpec ts = Type(p);
	if (ts < TS_SYMBOL && (ts!=TS_CONS || p))
		return;
	CObMap::CVal& val = m_obMap.m_arP[g_arTS[ts]][AsIndex(p)];
	if (val.Flag)
		return;
	val.Flag = true;
	switch (ts)
	{
	case TS_FRAME_PTR:
		Throw(E_FAIL);
	case TS_CONS:
	case TS_RATIO:
	case TS_COMPLEX:
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	case TS_RANDOMSTATE:
#endif
		{
			CConsValue *cons = Lisp().AsCons(p);
			Verify(cons->m_car);
			Verify(cons->m_cdr);
		}
		break;
	case TS_SYMBOL:
		{
			CSymbolValue *sv = ToSymbol(p);
			Verify(sv->GetFun());
			Verify(sv->GetPropList());
			Verify(sv->m_dynValue);
		}
		break;
	case TS_INTFUNC:
		{
			CIntFuncValue *ifv = Lisp().ToIntFunc(p);
			Verify(ifv->m_name);
			Verify(ifv->m_docstring);
			Verify(ifv->m_body);
			Verify(ifv->m_vars);
			Verify(ifv->m_optInits);
			Verify(ifv->m_keywords);
			Verify(ifv->m_keyInits);
			Verify(ifv->m_auxInits);
			Verify(ifv->m_env.m_varEnv);
			Verify(ifv->m_env.m_funEnv);
			Verify(ifv->m_env.m_blockEnv);
			Verify(ifv->m_env.m_goEnv);
			Verify(ifv->m_env.m_declEnv);
		}
		break;
	case TS_HASHTABLE:
		{
			CHashMap& m = *Lisp().ToHashTable(p)->m_pMap;
			Verify(m.m_func);
			for (CHashMap::iterator i=m.begin(); i!=m.end(); ++i) {
				Verify(i->first);
				Verify(i->second);
			}
		}
		break;
	case TS_STRUCT:
	case TS_OBJECT:
	case TS_ARRAY:
	case TS_CCLOSURE:
		{
			CArrayValue *av = Lisp().ToArray(p);
			Verify(av->m_displace);
			Verify(av->m_fillPointer);
			Verify(av->m_dims);
			if (!av->m_displace && av->m_elType == ELTYPE_T) {
				DWORD_PTR size = 1;
				for (CP p=av->m_dims; p;) {
					CConsValue *cons = Lisp().AsCons(p);
					size *= AsNumber(cons->m_car);
					p = cons->m_cdr;
				}
				for (int i=0; i<size; i++)
					Verify(av->m_pData[i]); 
			}
		}
		break;  
	case TS_PACKAGE:
		{
			CPackage *pack = ToPackage(p);
			Verify(pack->m_useList);  //!!!
			Verify(pack->m_docString);
		}
		break;
	}
}

void CVerifier::VerifySymbol(CSymbolValue *sv) {
	Verify(sv->GetFun());
	Verify(sv->GetPropList());
	Verify(sv->m_dynValue);
}

struct CVerifyCallback {
	CVerifier *m_v;

	void operator()(const pair<CSymbolValue*, byte>& pp) {
		m_v->VerifySymbol(pp.first);
	}
};

void CLispEng::Verify(CObMap& obMap) {
	CVerifier verifier(obMap);
	/*!!!
	{ //!!!
	for (int i=0; i<m_consMan.m_size; i++)
	{
	CConsValue *p = m_consMan.m_pBase+i;
	if (*(byte*)p != 0xFF)
	{
	if (Type(p->m_cdr) == TS_FRAME_PTR)
	Throw(E_FAIL);
	}
	}
	}*/
	for (int i=0; i<m_packageMan.m_size; i++) {
		CPackage *pack = m_packageMan.Base+i;
		if (*(byte*)pack != 0xFF) {
			CVerifyCallback c = { &verifier };
			pack->m_mapSym.ForEachSymbolValue(c);
		}
	}
}

#ifdef _DEBUG//!!!D
int g_b; 
#endif

} // Lisp::


