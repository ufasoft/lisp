#include <el/ext.h>

#include "lispeng.h"

namespace Lisp {

class CIndent {
public:
	CIndent(int off) {
		Init(off);
	}

	CIndent(const char *sz);

	CIndent(size_t n, const char *psz);
private:
	CDynBindFrame m_bindFrame;

	void Init(int off);
};

CIndent::CIndent(const char *sz) {
	CLispEng& lisp = Lisp();
	lisp.WritePChar(sz);
	Init(strlen(sz));
}

CIndent::CIndent(size_t n, const char *psz) {
	CLispEng& lisp = Lisp();
	ostringstream os;
	os << '#' << n << psz;
	string s = os.str();
	lisp.WritePChar(s.c_str());
	Init(s.length());
}

void CIndent::Init(int off) {
	CLispEng& lisp = Lisp();
	if (CStreamValue *sv = lisp.CastToPPStream(lisp.m_stm)) {
		m_bindFrame.Bind(lisp, S(L_S_PRIN_L1), CreateFixnum(AsFixnum(lisp.Spec(L_S_PRIN_L1))+off));
		m_bindFrame.Bind(lisp, S(L_S_PRIN_LM), CreateFixnum(AsFixnum(lisp.Spec(L_S_PRIN_LM))+off));
		m_bindFrame.Finish(lisp, 2);
	}
	else
		m_bindFrame.Finish(lisp, 0);
}

class CJustify {
public:
	CBool m_bLinear;

	CJustify(int trail);
	~CJustify();
private:
	CDynBindFrame m_bindFrame;
	size_t m_nPos;
	CBool m_bMultiLine;
};

CJustify::CJustify(int trail) {
	CLispEng& lisp = Lisp();
	CP ptrail = lisp.Spec(L_S_PRIN_TRAILLENGTH);
	ptrail = CreateFixnum(AsNumber(ptrail)+trail);
	m_bindFrame.Bind(lisp, S(L_S_PRIN_PREV_TRAILLENGTH), ptrail);
	m_bindFrame.Bind(lisp, S(L_S_PRIN_TRAILLENGTH), V_0);
	int count = 2;
	if (CStreamValue *sv = lisp.CastToPPStream(lisp.m_stm)) {
		m_bindFrame.Bind(lisp, S(L_S_PRIN_JBSTRINGS), sv->m_out);
		m_bindFrame.Bind(lisp, S(L_S_PRIN_JBLOCKS), 0);
		count += 2;
		m_bMultiLine = sv->m_bMultiLine;
		m_nPos = sv->LinePosRef();
		lisp.JustifyEmpty1();
	}
	m_bindFrame.Finish(lisp, count);
}

CJustify::~CJustify() {
	CLispEng& lisp = Lisp();
	if (lisp.m_bUnwinding)
		return;
	if (CStreamValue *sv = lisp.CastToPPStream(lisp.m_stm)) {
		lisp.JustifyEmpty();

		sv = lisp.AsStream(lisp.m_stm);
		DWORD nPos = exchange(sv->LinePosRef(), m_nPos);
		sv->m_out = lisp.Spec(L_S_PRIN_JBSTRINGS);
		sv->m_bMultiLine = m_bMultiLine;


		CP blocks = lisp.NReverse(lisp.Spec(L_S_PRIN_JBLOCKS));
		lisp.Push(blocks);
		if (m_bLinear) {
			intptr_t need = 0;
			CP rm = 0;
			for (CP car, b=blocks; lisp.SplitPair(b, car);)
				if (ConsP(car))
					goto MULTI_LAB;
				else
					need += lisp.SeqLength(car);
			need += AsNumber(lisp.Spec(L_S_PRIN_L1)) + AsNumber(lisp.Spec(L_S_PRIN_PREV_TRAILLENGTH));
			rm = lisp.RightMargin();
			if (rm && need>AsNumber(rm)+1)
MULTI_LAB:
				for (CP block; lisp.SplitPair(blocks, block);) {
					if (ConsP(block))
						lisp.MultiLineSubBlockOut(block);
					else
						lisp.WriteStr(AsString(block));
					if (!blocks)
						break;
					lisp.PPNewLine();
					for (int n=AsNumber(lisp.Spec(L_S_PRIN_LM)); n--;)
						lisp.WriteChar(' ');
					if (lisp.CheckLinesLimit())
						break;
				}
			else
				for (CP car; lisp.SplitPair(blocks, car);) {				 
					lisp.WriteStr(AsString(car));
					if (blocks)
						lisp.WriteChar(' ');
				}
		} else {
			for (CP block; lisp.SplitPair(blocks, block);) {
				bool bNewLine = true;
				if (ConsP(block)) {
					lisp.MultiLineSubBlockOut(block);
					lisp.AsStream(lisp.m_stm)->m_bMultiLine = true;
					if (!blocks)
					{
						lisp.AsStream(lisp.m_stm)->LinePosRef() = nPos;
						break;
					}
				} else {
					lisp.WriteStr(AsString(block));
					if (!blocks)
						break;
					if (!ConsP(block = Car(blocks)))
					{
						CP rm = lisp.RightMargin();
#ifdef X_DEBUG//!!!D
						CStreamValue *dbg_sv = lisp.AsStream(lisp.m_stm);
						int dbg_len = lisp.SeqLength(block);
						CP dbg_cdr = Cdr(blocks);
						CP dbg_prevtrail = lisp.Spec(L_S_PRIN_PREV_TRAILLENGTH);
#endif

						if (!rm ||
							lisp.AsStream(lisp.m_stm)->LinePosRef()
						    +lisp.SeqLength(block)
							+ (Cdr(blocks) ? 0 : AsNumber(lisp.Spec(L_S_PRIN_PREV_TRAILLENGTH)))
							  < AsNumber(rm))
						{
							  lisp.WriteChar(' ');
							  bNewLine = false;
						}
					}
				}
				if (bNewLine)
				{
					lisp.PPNewLine();
					for (int n=AsNumber(lisp.Spec(L_S_PRIN_LM)); n--;)
						lisp.WriteChar(' ');
				}
				if (lisp.CheckLinesLimit())
					break;
			}
		}
		lisp.SkipStack(1);
	}
}

class CPrintUnreadableKeeper : public CIndent {
public:
	CJustify m_j;

	CPrintUnreadableKeeper()
		:	CIndent("#<")
		,	m_j(1)
	{
//		if (lisp.Spec(L_S_PRINT_READABLY))
//			E_Error();
//		lisp.WritePChar("#<", m_stm);
	}

	~CPrintUnreadableKeeper() {
		CLispEng& lisp = Lisp();
		if (!lisp.m_bUnwinding)
			lisp.WriteChar('>');
	}
};

void CLispEng::MultiLineSubBlockOut(CP block) {
	block = NReverse(block);
	CP car;
	while (SplitPair(block, car), !StringP(car))
		;
	Push(block);
	WriteStr(AsString(car));
	Push(AsStream(m_stm)->m_out);
	F_NReconc();
	AsStream(m_stm)->m_out = m_r;
}

int CLispEng::FormatTab(CP stm, CP vec) {
	CArrayValue *sv = ToVector(vec);
	if (sv->GetVectorLength() != 4)
		E_Error(); //!!!
	Call(S(L_FORMAT_TAB), stm, sv->GetElement(0), sv->GetElement(1), sv->GetElement(2), sv->GetElement(3));
	return AsNumber(m_r);
}

void CLispEng::F_FormatTabulate() {
	CP colinc = ToOptional(Pop(), V_1);
	if (!colinc)
		colinc = V_1;
	CP colnum = ToOptional(Pop(), V_1);
	if (!colnum)
		colnum = V_1;
	CP stm = SV2;
	if (CStreamValue *sv = CastToPPStream(stm)) {
		CArrayValue *vec = CreateVector(4);
		vec->SetElement(1, Pop());
		vec->SetElement(0, Pop());
		vec->SetElement(2, colnum);
		vec->SetElement(3, colinc);
		CP q = sv->m_out;
		sv->m_out = Cons(FromSValue(vec), q);
		if (!StringP(Car(q)) || SeqLength(Car(q)))
			sv->m_out = Cons(CreatePPString(), sv->m_out);
		SkipStack(1);
	} else {
		Push(colnum, colinc);
		Funcall(S(L_FORMAT_TAB), 5);
		CStm sk(stm);
		for (int n=AsNumber(m_r); n--;)
			WriteChar(' ');
	}
	m_r = 0;
}

int CLispEng::PprintPrefix(bool bPrint, int indent) {
	int len = indent;
	CP prefix = Spec(L_S_PRIN_LINE_PREFIX);
	if (StringP(prefix)) {
		len += SeqLength(prefix);
		if (bPrint) {
			WriteStr(AsString(prefix));
			while (indent--)
				WriteChar(' ');
		}
	}
	return len;
}

CP CLispEng::RightMargin() {	
	CP prm = Spec(L_S_PRINT_RIGHT_MARGIN);
	if (!prm)
		prm = Spec(L_S_PRIN_LINELENGTH);
	if (prm)
		return CreateFixnum(max(int(AsNumber(prm)-PprintPrefix(false, 0)), 0));
	return 0;
}

bool CLispEng::StringFitLineP(CP stm, CP p, int off) {
	Push(stm);
	F_LinePosition();
	if (!PosFixNumP(m_r))
		return true;
	int pos = AsNumber(m_r);
	if (!PosFixNumP(m_r=RightMargin()))
		return true;
	int mar = AsNumber(m_r);
	int space = max(mar-pos, 0);
	CP car;
	SplitPair(p, car);
	int len = 0;
	if (StringP(car))
		len = SeqLength(car);
	else if (ConsP(car))
		return true;
	else if (VectorP(car)) {
		len = FormatTab(stm, car);
		while (p && !StringP(Car(p)))
			Inc(p);
		if (p)
			len += SeqLength(Car(p)); // string
		else
			return false;
	}
	return len+off <= space;
}

CP CLispEng::CreatePPString() {
	Push(CreateFixnum(50), S(L_CHARACTER), V_SP, V_U, 0);	// (make-array 50 :element-type 'character :fill-pointer 0)
	Push(V_0, 0, 0);
	F_MakeArray();
	return m_r;
}

CP CLispEng::ConsPPString(CStreamValue *stm, CP nlType) {
	CP v = Spec(L_S_PRIN_INDENTATION);
	if (v == V_U)
		v = V_0;
	CP cons = Cons(Cons(nlType, v), 0);
	if (stm && StringP(Car(stm->m_out))
			&& !SeqLength(Car(stm->m_out)))
	{
		AsCons(stm->m_out)->m_cdr = Cons(cons, Cdr(stm->m_out));
		return stm->m_out;
	} else {
		Push(cons);
		CP p = CreatePPString();
		p = Cons(p, Pop());
		if (stm) {
			AsCons(Cdr(p))->m_cdr = stm->m_out;
			stm->m_out = p;
		}
		return p;
	}	
}

void CLispEng::PPNewLine() {
	ConsPPString(AsStream(m_stm), 0);
	AsStream(m_stm)->LinePosRef() = 0;
	AsStream(m_stm)->m_bMultiLine = true;
	size_t n = AsPositive(Spec(L_S_PRIN_LINES));
	if (Spec(L_S_PRINT_LINES))
		SetSpecial(S(L_S_PRIN_LINES), CreateFixnum(n+1));
}

void CLispEng::JustifyEmpty1() {
	CP p = ConsPPString(0, 0);
	AsStream(m_stm)->m_out = p;
	AsStream(m_stm)->m_bMultiLine = false;
}

void CLispEng::JustifyEmpty() {
	CStreamValue *sv = AsStream(m_stm);
	CP cons = sv->m_bMultiLine ? Cons(sv->m_out, 0) : CP(sv->m_out);
	AsCons(cons)->m_cdr = Spec(L_S_PRIN_JBLOCKS); 
	SetSpecial(S(L_S_PRIN_JBLOCKS), cons);
}

void CLispEng::JustifySpace() {
	if (CastToPPStream(m_stm)) {
		JustifyEmpty();
		JustifyEmpty1();
		AsStream(m_stm)->LinePosRef() = AsNumber(Spec(L_S_PRIN_LM));
	} else
		WriteChar(' ');
}

void CLispEng::JustifyLast() {
	SetSpecial(S(L_S_PRIN_TRAILLENGTH), Spec(L_S_PRIN_PREV_TRAILLENGTH));
}

bool CLispEng::CheckLinesLimit() {
	CP lim = Spec(L_S_PRINT_LINES);
	if (Spec(L_S_PRINT_READABLY) || !PosFixNumP(lim))
		return false;
	CP now = Spec(L_S_PRIN_LINES);
	if (PosFixNumP(now) && AsFixnum(now)<AsFixnum(lim))
		return false;
	JustifyLast();	
	WritePChar("..");
	return true;
}

bool CLispEng::CheckLengthLimit(bool b) {
	if (!b)
		return false;
	JustifyLast();
	WritePChar("...");
	return true;
}

void CLispEng::PrNumber(CP p) {
	if (Type(p) != TS_COMPLEX)
		PrRealNumber(p);
	else {
		WritePChar("#C");
		CParen paren;
		CIndent indent(3);
		CJustify j(1);
		PrRealNumber(AsComplex(p)->m_real);
		JustifySpace();
		JustifyLast();
		PrRealNumber(AsComplex(p)->m_imag);
	}
}

void CLispEng::PrPair(CP car, CP cdr) {
	Push(car, cdr);
	CP *pStack = m_pStack;
	if (CPrinLevel plk=false) {
		CParen paren;
		CIndent ind(1);
		CJustify j(1);
		PrinObject(pStack[1]);
		JustifySpace();
		WriteChar('.');
		JustifySpace();
		JustifyLast();
		PrinObject(pStack[0]);
	}
	SkipStack(2);
}

void CLispEng::PrCons(CP p) {
	const char *pch;
	CConsValue *cons = AsCons(p);
	CP car = cons->m_car,
		 cdr = cons->m_cdr;
	bool bL1 = ListLengthIs1(cdr);
	int ind = 1;
	switch (car) {
	case S(L_QUOTE):
		{
			if (!bL1)
				break;
			WriteChar('\'');
			CIndent indent(1);
			PrinObject(Car(cdr));
		}
		return;
	case S(L_FUNCTION):
		{
			if (!bL1)
				break;
			CIndent ind("#\'");
			PrinObject(Car(cdr));
		}
		return;
	case S(L_BACKQUOTE):
		{
			if (!ConsP(cdr))
				break;
			CP cadr = Car(cdr);
			if ((ConsP(cdr=Cdr(cdr)) ? Cdr(cdr) : cdr))
				break;
			WriteChar('`');
			{
				CP bqlev = Spec(L_S_PRIN_BQLEVEL);
				if (Type(bqlev) != TS_FIXNUM)
					bqlev = V_0;
				CDynBindFrame bind;
				bind.Bind(_self, S(L_S_PRIN_BQLEVEL), CreateFixnum(AsNumber(bqlev)+1));
				bind.Finish(_self, 1);
				CIndent indent(1);
				PrinObject(cadr);
			}
		}
		return;
	case S(L_SPLICE):
		if (Type(Spec(L_S_PRIN_BQLEVEL))!=TS_FIXNUM || !bL1)
			break;
		pch = ",@";
		ind = 2;
		goto LAB_BQ;
	case S(L_NSPLICE):
		if (Type(Spec(L_S_PRIN_BQLEVEL))!=TS_FIXNUM || !bL1)
			break;
		pch = ",.";
		ind = 2;
		goto LAB_BQ;
	case S(L_UNQUOTE):
		if (Type(Spec(L_S_PRIN_BQLEVEL))!=TS_FIXNUM || !bL1)
			break;
		pch = ",";
		goto LAB_BQ;
	}
	if (CPrinLevel plk=false) {
		size_t lim = GetLengthLimit(),
					len = 0;
		CParen paren;
		CIndent indent(1);
		CJustify j(1);
		while (true) {
			if (!cdr) {
				cdr = car;
				goto LAB_LAST;
			}
			PrinObject(car);
			len++;
			JustifySpace();
			if (!ConsP(cdr))
				break;
			if (CheckLengthLimit(len >= lim) || CheckLinesLimit())
				return;
			if (CircleP(cdr, 0))
				break;
			cons = AsCons(cdr);
			car = cons->m_car,
			cdr = cons->m_cdr;
		}
		WriteChar('.');
		JustifySpace();
LAB_LAST:
		JustifyLast();
		PrinObject(cdr);
	}
	return;
LAB_BQ:
	size_t lev = AsPositive(Spec(L_S_PRIN_BQLEVEL));
	CDynBindFrame bind;
	bind.Bind(_self, S(L_S_PRIN_BQLEVEL), CreateFixnum(lev+1));
	bind.Finish(_self, 1);
	CIndent indent(ind);
	PrinObject(Car(cdr));
}

void CLispEng::PrVector(CP p) {
	if (CPrinLevel plk=false) {
		size_t lim = GetLengthLimit(),
					len = AsArray(p)->GetVectorLength();
		WriteChar('#');
		CParen paren;
		CIndent ind(2);
		CJustify j(1);
		for (size_t i=0; i<len; ++i) {
			if (i)
				JustifySpace();
			if (CheckLengthLimit(i >= lim) || CheckLinesLimit())
				break;
			if (i == len-1)
				JustifyLast();
			PrinObject(AsArray(p)->GetElement(i));				
		}
	}
}

void CLispEng::PrArrayRec(size_t depth, CP restDims) {
	++depth;
	CP dim;
	if (SplitPair(restDims, dim)) {
		if (CPrinLevel plk=false) {
			CParen paren;
			CIndent ind(1);
			CJustify j(1);
			size_t len = AsPositive(dim);
			size_t lim = GetLengthLimit();
			for (size_t i=0; i<len; i++) {
				if (i)
					JustifySpace();
				if (CheckLengthLimit(i >= lim) || CheckLinesLimit())
					break;
				if (i == len-1)
					JustifyLast();
				Push(CreateFixnum(i));
				PrArrayRec(depth, restDims);
				SkipStack(1);
			}
		}
	} else {
		m_pStack -= depth;
		memcpy(m_pStack, m_pStack+depth, sizeof(CP)*depth);
		F_Aref(depth-1);
		PrinObject(m_r);
	}
}

void CLispEng::PrArray(CP p) {
	CArrayValue *ar = AsArray(p);
	size_t rank = ar->GetRank();
	size_t depth = rank;
	bool bReadable = true;

	if (bReadable) {
		WritePChar("#A");
		CParen paren;
		CIndent ind(3);
		Push(p);
		F_ArrayElementType();
		Push(m_r);
		PrinObjectDispatch(m_r);
		SkipStack(1);
		WriteChar(' ');
		PrCons(AsArray(p)->m_dims);
		WriteChar(' ');
		Push(p);
		PrArrayRec(0, AsArray(p)->m_dims);
		SkipStack(1);
	} else {
		CIndent indent(rank, "A");
		PrArrayRec(depth, rank);
	}
}

void CLispEng::PrCircle(CP p) {
	CCircleInfo ci;
	if (CircleP(p, &ci)) {
		if (ci.m_bFirst) {
			CIndent ind(ci.m_idx, "=");
			PrinDispatch(p);
		} else {
			WriteChar('#');
			WriteUint(ci.m_idx);
			WriteChar('#');
		}
	}
	else
		PrinDispatch(p);
}

void CLispEng::PrOtherObj(RCString obj, RCString s) {
	CPrintUnreadableKeeper puk;
	WriteStr(obj);
	WriteChar(' ');
	WriteStr(s);
}

void CLispEng::PrOtherObj(CP obj, RCString s) {
	CPrintUnreadableKeeper puk;
	PrinObject(obj);
	WriteChar(' ');
	WriteStr(s);
}

class CExternalPrint : public CDynBindFrame {
public:
	CExternalPrint();
};

static const struct CSymBoundVal {
	CLispSymbol sym;
	CP val;
} s_boundVals[] = {
	{ ENUM_L_S_PRINT_ESCAPE,		V_T },
	{ ENUM_L_S_PRINT_BASE,			V_10 },
	{ ENUM_L_S_PRINT_RADIX,			V_T },
	{ ENUM_L_S_PRINT_CIRCLE,		V_T },
	{ ENUM_L_S_PRINT_LEVEL,			0 },
	{ ENUM_L_S_PRINT_LENGTH,		0 },
	{ ENUM_L_S_PRINT_LINES,			0 },
	{ ENUM_L_S_PRINT_MISER_WIDTH,	0 },
	{ ENUM_L_S_PRINT_PPRINT_DISPATCH,	0 },
	{ ENUM_L_S_PRINT_GENSYM,		V_T },
	{ ENUM_L_S_PRINT_ARRAY,			V_T },
	{ ENUM_L_S_PRINT_CLOSURE,		V_T }
};

CExternalPrint::CExternalPrint() {
	CLispEng& lisp = Lisp();
	int count = 1;
	if (!lisp.Spec(L_S_PRINT_CIRCLE) && lisp.Spec(L_S_PRINT_CIRCLE_TABLE)!=V_U) {
		Bind(lisp, S(L_S_PRINT_CIRCLE_TABLE), V_U);
		++count;
	}
	if (lisp.Spec(L_S_PRINT_READABLY)) {
		for (int i=0; i<size(s_boundVals); ++i) {
			const CSymBoundVal& sv = s_boundVals[i];
			if (lisp.get_Special(sv.sym) != sv.val) {
				Bind(lisp, IDX_TS(sv.sym, TS_SYMBOL), sv.val);
				++count;
			}
		}
	}
	Bind(lisp, S(L_S_PRIN_STREAM), lisp.m_stm);
	Finish(lisp, count);
}

void CLispEng::PrInstance(CP p) {
	if (Spec(L_S_COMPILING) && Spec(L_S_PRINT_READABLY) && Spec(L_S_LOAD_FORMS)) {
		Call(S(L_MAKE_INIT_FORM), p);
		if (m_r) {
			Push(p = m_r);
			{
				CIndent ind("#.");
				PrinObject(p);
			}
			SkipStack(1);
			return;
		}
	}
	if (CPrinLevel plk=false) {
		CExternalPrint ep;
		Call(S(L_PRINT_OBJECT), p, m_stm);
	}
}

void CLispEng::PrCClosureCodeVector(CP p) {
	size_t len = AsArray(p)->DataLength;
	CIndent ind(len, "Y(");
	CJustify j(1);
	for (size_t i=0; i<len; ++i) {
		if (i)
			JustifySpace();
		int el = AsIndex(AsArray(p)->GetElement(i));
#if UCFG_USE_POSIX
		WritePChar(Convert::ToString(el, 16));
#else
		char buf[20];
		_ltoa(el, buf, 16);
		WritePChar(buf);
#endif
	}
	WriteChar(')');
}

void CLispEng::PrCClosure(CP p) {
	if (Spec(L_S_PRINT_READABLY) || Spec(L_S_PRINT_CLOSURE)) {
		if (CPrinLevel plk=false) {
			WritePChar("#Y");
			CParen paren;
			CIndent ind(3);
			CJustify j(1);
			PrinObject(TheClosure(p).NameOrClassVersion);
			JustifySpace();
			{
				CPrintDispatchKeeper pdk(&CLispEng::PrCClosureCodeVector);
				PrCircle(TheClosure(p).CodeVec);
			}
			JustifySpace();
			PrinObject(0); // seclass
			size_t len = AsArray(p)->DataLength;
			for (size_t i=0; i<len; i++) {
				JustifySpace();
				PrinObject(AsArray(p)->m_pData[i]);
			}
		}
	}
	else
		PrOtherObj(AsTrueString(TheClosure(p).NameOrClassVersion), "COMPILED-FUNCTION"); //!!! may be not string
}

void CLispEng::F_WriteUnreadable() {
	bool bId = ToOptionalNIL(Pop()),
			 bType = ToOptionalNIL(Pop());
	CP stm = SV,
		obj = SV1,
		fun = SV2;
	CStm sk(stm);
	{
		CPrintUnreadableKeeper puk;
		if (bType) {
			Push(obj);
			F_TypeOf();
			CStm sk(stm);
			Prin1(m_r);
			if (fun || bId)
				JustifySpace();
		}
		if (fun)
			Funcall(fun, 0);
		if (bId) {
			if (fun || !bType)
				JustifySpace();
#if UCFG_USE_POSIX
			WritePChar("0x"+Convert::ToString(obj, 16));
#else
			char buf[20] = "0x";
			_ltoa(obj, buf+2, 16);
			WritePChar(buf);
#endif
		}
	}
	ClearResult();
	SkipStack(3);
}

void CLispEng::PrinObjectDispatch(CP p) {
	switch (Type(p)) {
	case TS_SPECIALOPERATOR:
		PrOtherObj(s_stSOInfo[AsIndex(p)].m_name, "SPECIAL-OPERATOR");
		break;
	case TS_SUBR:
		{
			CP subrName = GetSubrName(p);
			if (Spec(L_S_PRINT_READABLY)) {
				if (!Spec(L_S_READ_EVAL))
					E_Error();
				WritePChar("#.");
				CParen paren;
				PrSymbol(S(L_FIND_SUBR));
				WritePChar(" \'");
				PrinObject(subrName);
			}
			else
				PrOtherObj(subrName, "SYSTEM-FUNCTION");
		}
		break;
	case TS_INTFUNC:
		{
			CPrintUnreadableKeeper puk;
			WritePChar("FUNCTION ");
			PrinObject(AsIntFunc(p)->m_name);
			WriteChar(' ');
			PrinObject(AsIntFunc(p)->m_form);
		}
		break;
	case TS_CONS:
		if (p) {
			PrCons(p);
			break;
		}
	case TS_SYMBOL:
		PrSymbol(p);
		break;
	case TS_PATHNAME:
		if (Spec(L_S_PRINT_READABLY))
			Call(S(L_PRINT_PATHNAME), p, m_stm);
		else {
			if (Spec(L_S_PRINT_ESCAPE))
				WritePChar("#P");
			Call(S(L_NAMESTRING), p);
			Push(m_r);
			PrString(m_r);
			SkipStack(1);
		}
		break;
	case TS_ARRAY:
		if (StringP(p))
			PrString(p);
		else if (!Spec(L_S_PRINT_READABLY) && !Spec(L_S_PRINT_ARRAY)) {
			CPrintUnreadableKeeper puk;
			WritePChar("ARRAY");
		} else {
			if (VectorP(p)) {
				if (AsArray(p)->GetElementType() == ELTYPE_BIT)
					PrBitVector(p);
				else
					PrVector(p);
			}
			else
				PrArray(p);
		}
		break;
	case TS_FIXNUM:
	case TS_BIGNUM:
	case TS_FLONUM:
	case TS_RATIO:
	case TS_COMPLEX:
		PrNumber(p);
		break;
	case TS_CHARACTER:
		return Call(GetSymbol("_PRINT-CHARACTER", m_packSYS), p, m_stm); //!!!
	case TS_HASHTABLE:
		return Call(GetSymbol("_PRINT-HASH-TABLE", m_packSYS), p, m_stm); //!!!
	case TS_CCLOSURE:
		if (!FuncallableInstanceP(p))
			return PrCClosure(p);
#ifdef _X_DEBUG//!!!D
	case TS_STREAM:
		E_Error();
#endif
	case TS_FRAME_PTR:
		PrFramePtr(p);
		break;
	case TS_FRAMEINFO:
		PrFrameInfo(p);
		break;
	case TS_OBJECT:
		PrInstance(p);
		break;
#if UCFG_LISP_FFI
	case TS_FF_PTR:
		PrPointer(p);
		break;
#endif
	default: //!!!
		Call(S(L_PRINT_OBJECT), p, m_stm);
	}
}

void CLispEng::PPrintObjectDispatch(CP p) {
	if (CPrinLevel plk=false) {
		CExternalPrint ep;
		Call(Spec(L_S_PRIN_PPRINTER), m_stm, p);
	}
}

void CLispEng::PrinDispatch(CP p) {
	(this->*m_mfnPrintDispatch)(p);
}

void CLispEng::F_PPrintLogicalBlock() {
	CP stm = TestOStream(SV);
	CP p = SV1;
	if (Type(p) == TS_CONS) {
		CDynBindFrame dynBind(S(L_S_PRIN_PPRINTER), SV2, true);
		CPrintDispatchKeeper pdk(&CLispEng::PPrintObjectDispatch);
		PrEnter(stm, p);
	} else {	
		CPrintDispatchKeeper pdk(&CLispEng::PrinObjectDispatch);
		PrEnter(stm, p);
	}
	m_r = 0;
	SkipStack(3);
}

void CLispEng::F_PPrintIndent() {
	Call(S(L_ROUND), SV1);
	intptr_t off = AsNumber(m_r);
	CP indent = Spec(L_S_PRIN_INDENTATION);
	m_r = 0;
	Push(SV);
	F_LinePosition();
	CP linepos = m_r;
	switch (SV2) {
	case S(L_K_BLOCK):
		if (PosFixNumP(indent))
			off += AsNumber(indent);
		break;
	case S(L_K_CURRENT):
		if (linepos)
			off += AsNumber(linepos);
		break;
	default:
		E_TypeErr(SV2, 0);
	}
	if (CastToPPStream(SV) && Spec(L_S_PRINT_PRETTY)) {
		off = max(off, intptr_t(0));
		SetSpecial(S(L_S_PRIN_INDENTATION), CreateFixnum(off));
		CStm sk(SV);
		while (linepos++ < off)
			WriteChar(' ');
	}
	SkipStack(3);
	m_r = 0;
	m_cVal = 1;
}

void CLispEng::F_PPrintNewline() {
	CP stm = TestOStream(SV);
	switch (SV1) {
	case S(L_K_LINEAR):
	case S(L_K_FILL):
	case S(L_K_MISER):
	case S(L_K_MANDATORY):
		break;
	default:
		E_TypeErr(SV1, 0);
	}
	if (CStreamValue *sv = CastToPPStream(stm))
		if (Spec(L_S_PRINT_PRETTY)) {
			switch (SV1) {
			case S(L_K_MISER):
				if (!Spec(L_S_PRIN_MISERP))
					break;
				SV1 = S(L_K_LINEAR);
			case S(L_K_LINEAR):
				if (!sv->m_bMultiLine)
					goto LAB_FILL;
			case S(L_K_MANDATORY):
				SV1 = 0;				
			case S(L_K_FILL):
LAB_FILL:
				ConsPPString(sv, SV1);
			}
		}
		SkipStack(2);
}


} // Lisp::


