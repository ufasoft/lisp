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

void CLispEng::F_FuncallableInstanceP() {
	m_r = FromBool(FuncallableInstanceP(Pop()));
}

bool CLispEng::InstanceP(CP p) {
	return Type(p)==TS_OBJECT || FuncallableInstanceP(p);
}

bool CLispEng::InstanceOf(CP p, CP c) {
	return InstanceP(p) && GetHash(c, TheClass(TheClassVersion(TheInstance(p).ClassVersion).NewestClass).AllSuperclasses);
}

void CLispEng::F_StdInstanceP() {
	m_r = FromBool(InstanceP(Pop()));
}

void CLispEng::F_FunctionP() {
	switch (Type(Pop()))
	{
	case TS_INTFUNC:
	case TS_CCLOSURE:
	case TS_SUBR:
		m_r = V_T;
	}
}

void CLispEng::F_CompiledFunctionP() {
	switch (Type(SV))
	{
	case TS_CCLOSURE:
		if (FuncallableInstanceP(SV))
			break;
	case TS_SUBR:
		m_r = V_T;
	}
	SkipStack(1);
}

bool CLispEng::SomeClassP(CP p, CLispSymbol ls) {
	if (!InstanceP(p))
		return false;
	CP cv = TheInstance(p).ClassVersion;
	if (cv==Spec(L_S_CLASS_VERSION_STANDARD_CLASS) || cv==Spec(L_S_CLASS_VERSION_STRUCTURE_CLASS) ||
		cv==Spec(L_S_CLASS_BUILT_IN_STANDARD_CLASS))
		return true;
	CP cl = TheClassVersion(cv).NewestClass;
	CP r = GetHash(get_Special(ls), TheClass(cl).AllSuperclasses);
	m_r = 0;
	return r;
}

void CLispEng::F_PotentialClassP() {
	m_r = FromBool(SomeClassP(Pop(), ENUM_L_S_CLASS_POTENTIAL_CLASS));
}

bool CLispEng::DefinedClassP(CP p) {
	return SomeClassP(p, ENUM_L_S_CLASS_DEFINED_CLASS);
}

void CLispEng::F_DefinedClassP() {
	m_r = FromBool(DefinedClassP(Pop()));
}

void CLispEng::F_StructureObjectP() {
	m_r = FromBool(Type(Pop()) == TS_STRUCT);
}

void CLispEng::F_StructureTypeP() {
	CP st = Pop(),
		typ = Pop();
	if (Type(st) == TS_STRUCT)		
		m_r = FromBool(Memq(typ, TheStruct(st).Types));
}

void CLispEng::F_CopyStructure() {
	ToStruct(SV);
	size_t len = AsArray(SV)->DataLength;
	CArrayValue *av = CreateVector(len);
	memcpy(av->m_pData, AsArray(SV)->m_pData, len*sizeof(CP));
	m_r = FromSValueT(av, TS_STRUCT);
	TheStruct(m_r).Types = TheStruct(SV).Types;
	SkipStack(1);
}

CP& CLispEng::GetStructRef() {
	size_t idx = AsPositive(Pop());
	CStructInst *st = ToStruct(Pop());
	if (!Memq(Pop(), st->Types))
		E_Error();
	CArrayValue *av = (CArrayValue*)st;
	if (!idx || idx-1>=av->DataLength)
		E_Error();
	return av->m_pData[idx-1];
}

void CLispEng::F_StructureRef() {
	m_r = GetStructRef();
}

void CLispEng::F_StructureStore() {
	m_r = Pop();
	GetStructRef() = m_r;
}

void CLispEng::F_ClosureP() {
	m_r = FromBool(ClosureP(Pop()));
}

pair<CP, CP> CLispEng::GetFunctionName(CP fun) {
	switch (Type(fun))
	{
	case TS_INTFUNC: return pair<CP, CP>(CreateChar('C'), AsIntFunc(fun)->m_name);
	case TS_CCLOSURE:
		{
			CClosure& c = TheClosure(fun);
			return pair<CP, CP>(CreateChar('c'), AsArray(fun)->m_flags & FLAG_CLOSURE_INSTANCE ? c.Consts[1] : c.NameOrClassVersion);
		}
	case TS_SUBR:
		Push(CreateInteger(int(AsIndex(fun) & 0x1FF)));
		F_FunTabRef();
		return pair<CP, CP>(CreateChar('S'), m_r);
	default:
		E_Error();
	}
}

CP& CLispEng::ClosureName() {
	CP p = Pop();
	switch (Type(p))
	{
	case TS_INTFUNC:
		return AsIntFunc(p)->m_name;
	default:
#ifdef _DEBUG//!!!D
		Disassemble(cerr, CurClosure);
#endif
		E_Error();
	case TS_CCLOSURE:
		CClosure& c = TheClosure(p);
		return AsArray(p)->m_flags & FLAG_CLOSURE_INSTANCE ? c.Consts[1] : c.NameOrClassVersion;
	}
}

void CLispEng::F_ClosureName() {
	m_r = ClosureName();
}

void CLispEng::F_SetfClosureName() {
	CP& place = ClosureName();
	m_r = place = Pop();
}


void CLispEng::F_RecordRef() {
	size_t	idx = AsPositive(Pop());
	CP p = Pop(),
		*data;
	switch (Type(p))
	{
	case TS_OBJECT:
	case TS_STRUCT:
		{
			if (!idx)
			{
				m_r = TheInstance(p).ClassVersion;
				return;
			}
			idx--;
			CArrayValue *av = AsArray(p);
			if (idx >= av->DataLength)
				E_Error();
			data = av->m_pData;
		}
		break;
		//!!!	case TS_MACRO:
	case TS_INTFUNC:
		m_r = AsIntFunc(p)->GetField(idx);
		return;
	case TS_SYMBOLMACRO:
	case TS_GLOBALSYMBOLMACRO:
		if (idx != 0)
			E_Error();
		m_r = ToSymbolMacro(p)->m_macro;
		return;
	case TS_CCLOSURE:
		{
			CClosure& c = TheClosure(p);
			switch (idx)
			{
			case 0:
				m_r = c.NameOrClassVersion;
				return;
			case 1:
				m_r = c.CodeVec;
				return;
			}
			idx -= 2;
			CArrayValue *av = AsArray(p);
			if (idx >= av->DataLength) {
#ifdef _DEBUG//!!!D
				idx = idx;
#else
				E_Error();
#endif
			}
			data = av->m_pData;
		}
		break;
	default:
		E_Error();
	}
	m_r = data[idx];
}

/*!!!
void CLispEng::SetCodevecFromList(CP closure, CP p)
{
Push(closure, p);
CArrayValue *vec = CreateVector(Length(p)*8, ELTYPE_BIT);
AsClosure(SV1)->m_codevec = FromSValue(vec);
byte *pb = (byte*)vec->m_pData;
for (CP q; SplitPair(p, q);)
*pb++ = (byte)AsNumber(q);
SkipStack(2);
}
*/

void CLispEng::F_RecordStore() {
	CP val = Pop();
	size_t idx = AsPositive(Pop());
	//!!!	if (idx > 19)
	//!!!		E_Error(); //!!!
	CP p = Pop(),
		*data;
	switch (Type(p))
	{
	case TS_STRUCT:
	case TS_OBJECT:		
		if (!idx)
			data = &TheInstance(p).ClassVersion;
		else {
			idx--;
			CArrayValue *av = AsArray(p);
			if (idx >= av->DataLength)
				E_Error();
			data = (CP*)av->m_pData;
		}
		break;
	case TS_INTFUNC: data = (CP*)ToIntFunc(p); break;
	case TS_CCLOSURE:
		{
			CClosure& c = TheClosure(p);
			switch (idx)
			{
			case 0:
				E_Error();
			case 1:
				c.CodeVec = val;
				return;
			}
			data = ((CArrayValue*)&c)->m_pData;
			idx -= 2;
		}
		break;
	default:
		E_Error();
	}
	data[idx] = val;
}

void CLispEng::F_RecordLength() {
	CP p = Pop();
	size_t headerLen = 1;
	switch (Type(p))
	{
	case TS_CCLOSURE:
		headerLen = 2;
	case TS_OBJECT:
	case TS_STRUCT:
		break;
	default:
		E_Error();
	}
	m_r = CreateFixnum(headerLen+AsArray(p)->DataLength);
}

void CLispEng::F_ClosureConsts() {
	CArrayValue *vec = (CArrayValue*)ToCClosure(SV);
	size_t len = vec->DataLength;
	CP *data = vec->m_pData;
	{
		CListConstructor lc;
		for (size_t i=0; i<len; i++)
			lc.Add(data[i]);
		m_r = lc;
	}
	SkipStack(1);
}

void CLispEng::F_ClosureConst() {
	CArrayValue *vec = (CArrayValue*)ToCClosure(SV1);
	size_t idx = AsPositive(SV);
	if (idx >= vec->DataLength)
		E_Error();
	m_r = vec->m_pData[idx];
	SkipStack(2);
}

void CLispEng::F_SetClosureConst() {
	CArrayValue *vec = (CArrayValue*)ToCClosure(SV1);
	size_t idx = AsPositive(SV);
	if (idx >= vec->DataLength)
		E_Error();
	m_r = vec->m_pData[idx] = SV2;
	SkipStack(3);
}

void CLispEng::F_ClosureCodevec() {
	m_r = ToCClosure(Pop())->CodeVec;
}

void CLispEng::F_ClosureSetDocumentation() {
	ToCClosure(SV1);
//!!!TODO
	SkipStack(2);
}

void CLispEng::F_GenericFunctionP() {
	bool b = false;
	switch (Type(SV))
	{
	case TS_INTFUNC: b = AsIntFunc(SV)->m_bGeneric; break;
	case TS_CCLOSURE: b = TheClosure(SV).IsGeneric; break;
	}
	m_r = FromBool(b);
	SkipStack(1);
}

void CLispEng::F_SetFuncallableInstanceFunction() {
	CP clos = SV1;
	if (!FuncallableInstanceP(clos))
		E_Error();
	CP codevec,
		venv;
	switch (Type(SV))
	{
	case TS_CCLOSURE:
		if (AsArray(SV)->DataLength <= 1) {
			codevec = TheClosure(SV).CodeVec;
			venv = AsArray(SV)->DataLength==1 ? TheClosure(SV).VEnv : 0;
			break;
		}
	case TS_INTFUNC:
	case TS_SUBR:
		Call(S(L_MAKE_TRAMPOLINE), SV);
		codevec = m_r;
		venv = SV;
		break;
	default:
		E_Error();
	}
	TheClosure(clos).CodeVec = codevec;
	TheClosure(m_r=clos).VEnv = venv;
	SkipStack(2);
}

void CLispEng::F_CopyGenericFunction() {
	Push(SV);
	F_GenericFunctionP();
	if (!m_r)
		E_Error();
	CP nvenv = CopyVector(ToCClosure(SV)->VEnv);
	AsArray(nvenv)->SetElement(0, SV1);
	SV1 = nvenv;
	CClosure *c = CopyClosure(Pop());
	c->VEnv = Pop();
	m_r = FromSValueT(c, TS_CCLOSURE);
}

void CLispEng::F_GenericFunctionEffectiveMethodFunction() {
	if (!ToCClosure(SV)->IsGeneric)
		E_Error();
	m_r = FromSValueT(CopyClosure(SV), TS_CCLOSURE);
	CP vec = CopyVector(TheClosure(SV).CodeVec);
	TheClosure(m_r).CodeVec = vec;
	TheClosure(m_r).get_Header().m_flags |= 8;
}

void CLispEng::InitStdInstance(size_t len) {
	if (len < 1)
		E_Error();
	CArrayValue *av = CreateVector(len-1);
	((CInstance*)av)->ClassVersion = Pop();
	FillMem(av->m_pData, len-1, V_U);
	m_r = FromSValueT(av, TS_OBJECT);
}

void CLispEng::F_AllocateStdInstance() {
	size_t len = AsPositive(Pop());
	if (!DefinedClassP(SV))
		E_Error();
	CClass& cl = TheClass(SV);
	cl.Instantiated = V_T;
	SV = cl.CurrentVersion;
	InitStdInstance(len);
}

void CLispEng::F_AllocateMetaobjectInstance() {
	size_t len = AsPositive(Pop());
	if (ToVector(SV)->GetVectorLength()*sizeof(CP) != sizeof(CClassVersion))
		E_Error();
	InitStdInstance(len);
}

void CLispEng::F_AllocateFuncallableInstance() {
	size_t len = AsPositive(Pop());
	if (len < 4)
		E_Error();
	if (!DefinedClassP(SV))
		E_Error();
	CClosure *c = CreateClosure(len-2);
	CClass& cl = TheClass(Pop());
	cl.Instantiated = V_T;
	((CArrayValue*)c)->m_flags |= FLAG_CLOSURE_INSTANCE;
	c->NameOrClassVersion = cl.CurrentVersion;
	c->CodeVec = Spec(L_S_ENDLESS_LOOP_CODE);
	c->VEnv = 0;
	FillMem(c->Consts+1, len-3, V_U);
	m_r = FromSValueT(c, TS_CCLOSURE);
}

void CLispEng::F_MakeStructure() {
	CArrayValue *av = CreateVector(AsNumber(Pop())-1);
	((CStructInst*)av)->Types = Pop();
	m_r = FromSValueT(av, TS_STRUCT);
}

void CLispEng::AllocateInstance(CP clas) {
	CClass& cl = TheClass(clas);
	if (!ConsP(cl.CurrentVersion)) {
		Push(clas);
		if (cl.Initialized != V_6) {
			Call(S(L_FINALIZE_INHERITANCE), clas);
			ASSERT(cl.Initialized == V_6);
		}
		Push(cl.InstanceSize);
		cl.FuncallableP ? F_AllocateFuncallableInstance() : F_AllocateStdInstance();
	} else {
		Push(cl.CurrentVersion, cl.InstanceSize);
		F_MakeStructure();
		FillMem(AsArray(m_r)->m_pData, AsArray(m_r)->DataLength, V_U); //!!! was V_US
	}
}

void CLispEng::CheckInitArgList(size_t nArgs) {
	if (nArgs & 1)
		E_Error();
	while (nArgs) {
		nArgs -= 2;
		CP key = m_pStack[nArgs+1];
		if (key && Type(key)!=TS_SYMBOL)
			E_ProgramErr();
	}
}

void CLispEng::F_PAllocateInstance(size_t nArgs) {
	CheckInitArgList(nArgs);

	m_pStack += nArgs;
	AllocateInstance(Pop());
}

void CLispEng::KeywordTest(CP *pStack, size_t nArgs, CP vk) {
	if (vk == V_T)
		return;
	for (int i=0; i<nArgs; i+=2)
		if (pStack[-i-1] == S(L_K_ALLOW_OTHER_KEYS))
			if (pStack[-i-2])
				return;
			else
				break;
	for (int i=0; i<nArgs; i+=2) {
		CP key = pStack[-i-1];
		ToSymbol(key);
		if (key!=S(L_K_ALLOW_OTHER_KEYS) && !Memq(key, vk))
			E_Error();
	}
}

void CLispEng::F_PMakeInstance(size_t nArgs) {
	if (nArgs & 1)
		E_ProgramErr(IDS_E_KeywordArgsNoPairwise, Listof(nArgs));
	CP *pStack = m_pStack+nArgs;
	CP clas = pStack[0];
	if (Type(clas) != TS_OBJECT)
		E_TypeErr(clas, 0); //!!!
	if (TheClass(clas).Initialized != V_6) {
		Call(S(L_FINALIZE_INHERITANCE), clas);
		ASSERT(TheClass(clas).Initialized == V_6);
	}
	for (CP defIA=TheClass(clas).DefaultInitargs, car; SplitPair(defIA, car);) {
		CP key = Car(car);
		for (int i=0; i<nArgs; i+=2)
			if (pStack[-i-1] == key)
				goto LAB_FOUND;
		Push(key);
		{
			CP init = Car(Cdr(Cdr(car)));
			if (ClosureP(init)) {
				CClosure& c = TheClosure(init);
				if (c.NameOrClassVersion==S(L_CONSTANT_INITFUNCTION) && c.CodeVec==Spec(L_S_CONSTANT_INITFUNCTION_CODE)) {
					Push(AsArray(init)->GetElement(0));
					goto LAB_NEXT;
				}
			}
			Funcall(init, 0);
		}
		Push(m_r);
LAB_NEXT:
		nArgs += 2;
LAB_FOUND:
		;
	}
	if (CP info = GetHash(clas, Spec(L_S_MAKE_INSTANCE_TABLE))) {
		KeywordTest(pStack, nArgs, ToVector(info)->GetElement(0));
		Push(clas);
		for (int i=0; i<nArgs; i++)
			Push(pStack[-1-i]);
		Funcall(ToVector(info)->GetElement(1), nArgs+1);
		if (ClassOf(m_r) != clas)
			E_Error();
		pStack[0] = m_r;
		Push(m_r);
		memmove(m_pStack, m_pStack+1, nArgs*sizeof(CP));
		pStack[-1] = m_r;
		Funcall(ToVector(info)->GetElement(2), 1+nArgs);
		m_r = Pop();
	}
	else
		Funcall(S(L_INITIAL_MAKE_INSTANCE), 1+nArgs);
}

void CLispEng::UpdateInstance(CP p) {
#ifdef _DEBUG//!!!D
	E_Error();
#endif

	CInstance& inst = TheInstance(p);
	class CUpdatedKeeper {
		CP m_p;
	public:
		CUpdatedKeeper(CP p)
			:	m_p(p)
		{
			Lisp().AsArray(m_p)->m_flags |= FLAG_BeingUpdated;
		}

		~CUpdatedKeeper() { Lisp().AsArray(m_p)->m_flags &= ~FLAG_BeingUpdated; }
	} updatedKeeper(p);
	do {
		Push(p);
		CClassVersion *cv = &TheClassVersion(inst.ClassVersion);
		if (TheClass(TheClassVersion(cv->Next).Class).Initialized != V_6)
			E_Error();
		if (!cv->SlotListsValidP) {
			Call(S(L_CLASS_VERSION_COMPUTE_SLOTLISTS), inst.ClassVersion);
			cv = &TheClassVersion(inst.ClassVersion);
			ASSERT(cv->SlotListsValidP);
		}
		Push(cv->AddedSlots, cv->DiscardedSlots);

		// Fetch the values of the local slots that are discarded
		size_t count = 0;
		for (CP plist=cv->DiscardedSlotLocations; plist;) {
			CP slotName, slotInfo;
			SplitPair(plist, slotName);
			SplitPair(plist, slotInfo);
			CP val = ToArray(p)->m_pData[AsPositive(slotInfo)];
			if (val != V_U) {
				Push(slotName, val);
				count += 2;
			}
		}
		Push(Listof(count));

		// Fetch the values of the slots that remain local or were shared and become local. These values are retained
		CP oldClass = cv->Class,
			newClass = TheClassVersion(cv->Next).Class;
		size_t keptSlots = 0;
		for (CP plist=cv->KeptSlotLocations; plist;) {
			CP oldSlotInfo, newSlotInfo;
			SplitPair(plist, oldSlotInfo);
			SplitPair(plist, newSlotInfo);
			CP val = ConsP(oldSlotInfo) ? ToVector(TheClassVersion(Car(oldSlotInfo)).SharedSlots)->GetElement(AsPositive(Cdr(oldSlotInfo)))
				: ToVector(p)->GetElement(AsPositive(oldSlotInfo));
			if (val != V_U) {
				Push(val, newSlotInfo);
				keptSlots ++;
			}
		}
		size_t len = AsPositive(TheClass(newClass).InstanceSize);
		CArrayValue *av;
		CClass& clNew = TheClass(newClass);
		size_t dataSize = clNew.FuncallableP ? clNew.InstanceSize-2 : clNew.InstanceSize-1;
		byte elType = clNew.FuncallableP ? ELTYPE_BYTE : ELTYPE_T;
		av = AsArray(p);
		delete[] SwapRet(av->m_pData, CArrayValue::CreateData(elType, av->m_dims=CreateFixnum(dataSize), V_U));
		TheInstance(p).ClassVersion = newClass;
		if (clNew.FuncallableP)
			TheClosure(p).VEnv = 0;
		while (keptSlots--) {
			CP newSlotInfo = Pop();
			av->SetElement(AsPositive(newSlotInfo), Pop());
		}
		Funcall(S(L_UPDATE_INSTANCE_FOR_REDEFINED_CLASS), 4);
	}
	while (!InstanceValidP(p));
}

static const CP g_arVecSimple[5] = {S(L_SIMPLE_VECTOR), S(L_SIMPLE_BIT_VECTOR), S(L_SIMPLE_STRING), S(L_SIMPLE_BASE_STRING), S(L_SIMPLE_VECTOR)},
	g_arVec[5] = {S(L_VECTOR), S(L_BIT_VECTOR), S(L_STRING), S(L_BASE_STRING), S(L_VECTOR)};

void CLispEng::F_TypeOf() {
	CP i;
	CP p = Pop();
	switch (Type(p))
	{
	case TS_CHARACTER:
		{
			Push(p);
			F_StandardCharP();
			i = m_r ? S(L_STANDARD_CHAR)
				: AsChar(p)<256 ? S(L_BASE_CHAR) : S(L_EXTENDED_CHAR);
		}
		break;
	case TS_FIXNUM: i = p==V_0 || p==V_1? S(L_BIT) : S(L_FIXNUM); break;
	case TS_BIGNUM:     i = S(L_BIGNUM);       break;
	case TS_FLONUM:     i = S(L_SINGLE_FLOAT);  break;
	case TS_RATIO:      i = S(L_RATIO);        break;
	case TS_COMPLEX:    i = S(L_COMPLEX);      break;
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	case TS_RANDOMSTATE:i = S(L_RANDOM_STATE); break;
#endif
	case TS_CONS:
		i = p ? S(L_CONS) : S(L_NULL);
		break;
	case TS_SYMBOL: i = p==V_T ? S(L_BOOLEAN) : AsSymbol(p)->HomePackage==m_packKeyword ? S(L_KEYWORD) : S(L_SYMBOL); break;
	case TS_ARRAY:
		{
			CArrayValue *av = ToArray(p);
			if (VectorP(p))
				m_r = List((av->SimpleP() ? g_arVecSimple : g_arVec)[av->m_elType], av->m_dims);
			else
				m_r = List(av->SimpleP() ? S(L_SIMPLE_ARRAY) : S(L_ARRAY), av->m_dims, V_T);  //!!! must be type
		}    
		return;
	case TS_SUBR:       i = S(L_COMPILED_FUNCTION);         break;
	case TS_INTFUNC:    i = S(L_FUNCTION);      break;
	case TS_PACKAGE:    i = S(L_PACKAGE);      break;
	case TS_READTABLE:  i = S(L_READTABLE); break;
	case TS_HASHTABLE:  i = S(L_HASH_TABLE);    break;
	case TS_PATHNAME:   i = AsPathname(p)->LogicalP ? S(L_LOGICAL_PATHNAME) : S(L_PATHNAME);     break;
	case TS_WEAKPOINTER: i = S(L_WEAK_POINTER); break;
	case TS_STREAM:
		switch (AsStream(p)->m_subtype)
		{
		case STS_SYNONYM_STREAM:		i = S(L_SYNONYM_STREAM); break;
		case STS_TWO_WAY_STREAM:		i = S(L_TWO_WAY_STREAM); break;
		case STS_STRING_STREAM:			i = S(L_STRING_STREAM); break;
		case STS_FILE_STREAM:			i = S(L_FILE_STREAM); break;
		case STS_CONCATENATED_STREAM:	i = S(L_CONCATENATED_STREAM); break;
		case STS_BROADCAST_STREAM:		i = S(L_BROADCAST_STREAM); break;
		case STS_ECHO_STREAM:			i = S(L_ECHO_STREAM); break;		
		default:						i = S(L_STREAM);
		}
		break;
	case TS_STRUCT:
		m_r = Car(TheStruct(p).Types);
		return;
	case TS_CCLOSURE:
		if (!ClosureInstanceP(p)) {
			i = S(L_COMPILED_FUNCTION);
			break;
		}
	case TS_OBJECT:
		{
			CP clas = TheClassVersion(TheInstance(p).ClassVersion).NewestClass,
				name = TheClass(clas).Classname;
			m_r = Get(name, S(L_CLOSCLASS))==clas ? name : clas;
		}
		return;
	case TS_SPECIALOPERATOR: i = S(L_SPECIAL_OPERATOR); break;
	case TS_SYMBOLMACRO:				i = S(L_SYMBOL_MACRO); break;
	case TS_GLOBALSYMBOLMACRO: i = S(L_GLOBAL_SYMBOL_MACRO); break;
	case TS_MACRO:					i = S(L_MACRO);					break;
	case TS_FUNCTION_MACRO: i = S(L_FUNCTION_MACRO); break;
#if UCFG_LISP_FFI
	case TS_FF_PTR: i = S(L_FOREIGN_POINTER); break;
#endif
	default:
		E_ProgramErr();
	}
	m_r = i;
}

void CLispEng::F_ClassOf() {
	CP i;
	CP p = Pop();
	switch (Type(p))
	{
	case TS_CHARACTER:  i = S(L_CHARACTER);    break;
	case TS_FIXNUM:
	case TS_BIGNUM:
		i = S(L_INTEGER);
		break;
	case TS_FLONUM:     i = S(L_FLOAT);        break;
	case TS_RATIO:      i = S(L_RATIO);        break;
	case TS_COMPLEX:    i = S(L_COMPLEX);      break;
#if UCFG_LISP_BUILTIN_RANDOM_STATE
	case TS_RANDOMSTATE:i = S(L_RANDOM_STATE); break;
#endif
	case TS_CONS:
		i = p ? S(L_CONS) : S(L_NULL);
		break;
	case TS_SYMBOL:     i = S(L_SYMBOL);       break;
	case TS_ARRAY:
		{
			CArrayValue *av = ToArray(p);
			if (Type(av->m_dims) == TS_FIXNUM) {
				CP ar[5] = {S(L_VECTOR), S(L_BIT_VECTOR), S(L_STRING), S(L_STRING), S(L_VECTOR)};
				i = ar[av->m_elType];
			} else
				i = S(L_ARRAY);
		}
		break;
	case TS_SUBR:
	case TS_INTFUNC:
		i = S(L_FUNCTION);
		break;
	case TS_STREAM:
		Push(p);
		F_TypeOf();
		m_r = GetSymProp(m_r, S(L_CLOSCLASS));
		return;
	case TS_PACKAGE:    i = S(L_PACKAGE);      break;
	case TS_READTABLE:  i = S(L_READTABLE); break;
	case TS_HASHTABLE:  i = S(L_HASH_TABLE);    break;
	case TS_PATHNAME:    i = AsPathname(p)->m_dev == V_U ? S(L_LOGICAL_PATHNAME) : S(L_PATHNAME);     break;
	case TS_STRUCT:
		{
			CStructInst& str = TheStruct(p);
			for (CP types=str.Types, name; SplitPair(types, name);)
				if (DefinedClassP(m_r=GetSymProp(name, S(L_CLOSCLASS))))
					return;
		}
		m_r = V_T;
		return;
	case TS_CCLOSURE:
		if (!ClosureInstanceP(p)) {
			i = S(L_FUNCTION);
			break;
		}
	case TS_OBJECT:
		m_r = TheClassVersion(TheInstance(p).ClassVersion).NewestClass;
		return;
	case TS_GLOBALSYMBOLMACRO:
	case TS_SPECIALOPERATOR:
	case TS_MACRO:
	case TS_SYMBOLMACRO:
	case TS_READLABEL:
	case TS_WEAKPOINTER:
#if UCFG_LISP_FFI
	case TS_FF_PTR:
#endif
		i = S(L_T);
		break;
	default:
		E_Error();
	}
	m_r = GetSymProp(i, S(L_CLOSCLASS));
	ASSERT(m_r);
}

CP CLispEng::ClassOf(CP p) {
	Push(p);
	if (InstanceP(p)) {
		CArrayValue *av = AsArray(Pop());
		if (!(av->m_flags & FLAG_BeingUpdated)) {
			InstanceUpdate(p);
			return TheClassVersion(TheInstance(p).ClassVersion).NewestClass;
		}
		return TheClassVersion(TheInstance(p).ClassVersion).Class;
	} else
		F_ClassOf();
	return m_r;
}

void CLispEng::F_PChangeClass() {
	AllocateInstance(SV);
	Push(m_r);
	CP oldClass = ClassOf(SV2);
	TheInstance(SV).ClassVersion = oldClass;
	CArrayValue *avCopy = AsArray(SV),
		*avOrig = AsArray(SV2);
	swap(avCopy->m_pData, avOrig->m_pData);
	swap(avCopy->m_dims, avOrig->m_dims);
	TheInstance(SV2).ClassVersion = SV1;
	m_r = Pop();
	SkipStack(2);
}

CP& CLispEng::PtrToSlot(CP inst, CP slotInfo) {
	if (ConsP(slotInfo))
		return ToArray(TheClassVersion(Car(slotInfo)).SharedSlots)->m_pData[AsPositive(Cdr(slotInfo))];
	size_t idx = AsPositive(slotInfo);
	switch (Type(inst))
	{
	case TS_OBJECT:
	case TS_STRUCT:
		return idx ? AsArray(inst)->m_pData[idx-1] : TheInstance(inst).ClassVersion;
	case TS_CCLOSURE:
		switch (idx)
		{
		case 0: return TheClosure(inst).NameOrClassVersion;
		case 1: return TheClosure(inst).CodeVec;
		default: return AsArray(inst)->m_pData[idx-2];
		}
	default:
		E_Error(); //!!! NOTREACHED
	}
}

pair<CP, CP> CLispEng::GetSlotInfo(CP inst, CP slotname, CP fun, bool bArgInStack) {
	CP clas = ClassOf(inst);
	if (CP slotInfo = GetHash(slotname, TheClass(clas).SlotLocationTable))
		return pair<CP, CP>(clas, slotInfo);
	Push(clas, inst, slotname, fun);
	if (bArgInStack)
		Push(SV4);
	Funcall(S(L_SLOT_MISSING), 4+bArgInStack);
	return pair<CP, CP>(clas, 0);
}

void CLispEng::F_SlotValue() {
	pair<CP, CP> pp = GetSlotInfo(SV1, SV, S(L_SLOT_VALUE), false);
	if (CP slotInfo = pp.second) {
		CP clas = pp.first;
		if (Type(slotInfo) == TS_OBJECT)
			Call(TheSlotDefinition(slotInfo).EfmSvuc, clas, SV1, slotInfo);
		else if ((m_r=PtrToSlot(SV1, slotInfo)) == V_U)
			Call(S(L_SLOT_UNBOUND), clas, SV1, SV);
	}
	SkipStack(2);
	m_cVal = 1;	
}

void CLispEng::F_SetSlotValue() {
	pair<CP, CP> pp = GetSlotInfo(SV2, SV1, S(L_SETF), true);
	if (CP slotInfo = pp.second) {
		if (Type(slotInfo) == TS_OBJECT)
			Call(TheSlotDefinition(slotInfo).EfmSsvuc, SV, pp.first, SV2, slotInfo);
		else
			PtrToSlot(SV2, slotInfo) = SV;
	}
	m_r = SV;
	SkipStack(3);
	m_cVal = 1;
}

void CLispEng::F_SlotBoundP() {
	pair<CP, CP> pp = GetSlotInfo(SV1, SV, S(L_SLOT_BOUNDP), false);
	if (CP slotInfo = pp.second) {
		if (Type(slotInfo) == TS_OBJECT)
			Call(TheSlotDefinition(slotInfo).EfmSbuc, pp.first, SV1, slotInfo);
		else
			m_r = FromBool(PtrToSlot(SV1, slotInfo) != V_U);
	} else
		m_r = FromBool(m_r);
	SkipStack(2);
	m_cVal = 1;	
}

void CLispEng::F_SlotMakUnbound() {
	pair<CP, CP> pp = GetSlotInfo(SV1, SV, S(L_SLOT_MAKUNBOUND), false);
	if (CP slotInfo = pp.second) {
		if (Type(slotInfo) == TS_OBJECT)
			Call(TheSlotDefinition(slotInfo).EfmSmuc, pp.first, SV1, slotInfo);
		else
			PtrToSlot(SV1, slotInfo) = V_U;
	}
	m_r = SV1;
	SkipStack(2);
}

void CLispEng::F_SlotExistsP() {
	m_r = FromBool(GetHash(SV, TheClass(ClassOf(SV1)).SlotLocationTable));
	SkipStack(2);
}

CP& CLispEng::SlotAccess() {
	if (!InstanceP(SV1))
		E_TypeErr(SV1, S(L_STANDARD_OBJECT));
	CArrayValue *av = AsArray(SV1);
	if (!(av->m_flags & FLAG_BeingUpdated))
		InstanceUpdate(SV1);
	CP slotInfo = Pop();
	if (!slotInfo)
		E_Error();
	return PtrToSlot(Pop(), slotInfo);
}

void CLispEng::F_StandardInstanceAccess() {
	m_r = SlotAccess();
}

void CLispEng::F_SetfStandardInstanceAccess() {
	CP& place = SlotAccess();
	place = m_r = Pop();
}

void CLispEng::F_PUnbound() {
	m_r = V_U;
}

CP& CLispEng::SlotUsingClass() {
	CP clas = ClassOf(SV1);
	if (clas != SV2)
		E_Error();
	m_cVal = 1;
	return PtrToSlot(SV1, TheSlotDefinition(SV).Location);
}

void CLispEng::F_PSlotValueUsingClass() {
	if ((m_r=SlotUsingClass()) != V_U)
		SkipStack(3);
	else {
		SV = TheSlotDefinition(SV).Name;
		Funcall(S(L_SLOT_UNBOUND), 3);
	}
	m_cVal = 1;
}

void CLispEng::F_PSetSlotValueUsingClass() {
	m_r = SlotUsingClass() = SV3;
	SkipStack(4);
}

void CLispEng::F_PSlotBoundpUsingClass() {
	m_r = FromBool(SlotUsingClass() != V_U);
	SkipStack(3);
}

void CLispEng::F_PSlotMakunboundUsingClass() {
	SlotUsingClass() = V_U;
	m_r = SV1;
	SkipStack(3);
}

void CLispEng::F_ClassGethash() {
	F_ClassOf();
	CP ht = Pop();
	Push(m_r, ht, 0);
	F_GetHash();
}



} // Lisp::


