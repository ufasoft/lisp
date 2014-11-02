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

/*!!!R
size_t CLispEng::HashString(RCString s) {
	size_t sum = 0;
	for (int i=0; i<s.Length; i++)
		sum += s[i];
	return sum;
}
*/

CHashTable::CHashTable()
	:	m_pMap(new CHashMap)
{
	m_type = TS_HASHTABLE;
}


void CHashMap::SetTestFun(CP test) {
	CLispEng& lisp = Lisp();
	if (test == g_fnEq)
		test = S(L_EQ);
	else if (test == g_fnEql)
		test = S(L_EQL);
	else if (test == g_fnEqual)
		test = S(L_EQUAL);
	else if (test == g_fnEqualP)
		test = S(L_EQUALP);
	m_func = test;
}

void CHashMap::Write(BlsWriter& wr) {
	(wr << m_func).WriteSize(size());
	for (iterator i=begin(); i!=end(); ++i)
		wr << i->first << i->second;
}

void CHashMap::Read(const BlsReader& rd) {
	CSPtr test;
	rd >> test;
	SetTestFun(test);
	size_t size = rd.ReadSize();
	while (size--) {
		CSPtr key, val;
		rd >> key >> val;
		insert(make_pair(key, val));
	}
}

void CLispEng::F_GetHash() {
	CHashMap& m = *ToHashTable(SV1)->m_pMap;
	CHashMap::iterator i = m.find(SV2);
	m_r = (m_arVal[1]=FromBool(i!=m.end())) ? i->second : ToOptionalNIL(SV);
	SkipStack(3);
	m_cVal = 2;
}

CP CLispEng::GetHash(CP key, CP ht) {
	Push(key, ht, 0);
	F_GetHash();
	m_cVal = 1;
	return m_r;
}

#ifdef _DEBUG//!!!D
	int g_count;
#endif

void CLispEng::F_PutHash() {
#ifdef _X_DEBUG//!!!D
	if (Type(SV2)==TS_CONS && Type(Car(SV2))==TS_OBJECT && Type(Cdr(SV2))==TS_OBJECT)
		m_r = m_r;
#endif
	CHashMap& m = *ToHashTable(SV1)->m_pMap;
	m[SV2] = SV;
	m_r = Pop();
	SkipStack(2);

#ifdef _DEBUG//!!!D
	g_count++;
#endif
}

void CLispEng::F_RemHash() {
	ToHashTable(SV)->m_pMap->erase(SV1);
	SkipStack(2);
	ClearResult();
}
/*!!!
void CLispEng::F_MapHash()
{
CHashTable *ht = ToHashTable(GetStack(0));
Push(0);
for (CHashTable::CHashMap::CIterator i(m_map); i; i++)
SetStack(0, AppendEx(GetStack(0), i.Val));
for (CP car; SplitPair(m_pStack[0], car);)
{
Push(Car(car));
Push(Cdr(car));
Funcall(GetStack(4), 2);
}
SkipStack(3);
ClearResult();
}
*/

void CLispEng::F_HashTableSize() {
	m_r = CreateInteger((int)ToHashTable(Pop())->m_pMap->size()); //!!!
}

void CLispEng::F_HashTableRehashSize() {
	m_r = ToHashTable(Pop())->m_rehashSize;
}

void CLispEng::F_HashTableRehashThreshold() {
	m_r = ToHashTable(Pop())->m_rehashThreshold;
}

void CLispEng::F_HashTableTest() {
	m_r = ToHashTable(Pop())->m_pMap->m_func;
}

void CLispEng::F_ClrHash() {  
	ToHashTable(m_r = Pop())->m_pMap->clear();
}

void CLispEng::F_HashTableCount() {
	m_r = CreateInteger((int)ToHashTable(Pop())->m_pMap->size());
}

void CLispEng::F_HashTableIterator() {
	CHashMap& m = *ToHashTable(Pop())->m_pMap;
	CListConstructor lc;
#ifdef X_DEBUG//!!!D
	set<CP> sorted;
	for (CHashMap::iterator i=m.begin(), e=m.end(); i!=e; ++i)
		sorted.insert(i->first);
	for (set<CP>::iterator i=sorted.begin(), e=sorted.end(); i!=e; ++i)
		lc.Add(Cons(*i, m[*i]));
#else
	for (CHashMap::iterator i=m.begin(), e=m.end(); i!=e; ++i)
		lc.Add(Cons(i->first, i->second));
#endif
	m_r = Cons(0, lc);
}

void CLispEng::F_HashTableIterate() {
	CConsValue *cons = ToCons(Pop());
	if (cons->m_cdr) {
		CP car;
		SplitPair(cons->m_cdr, car);
		m_r = V_T;
		m_arVal[1] = Car(car);
		m_arVal[2] = Cdr(car);
		m_cVal = 3;
	}
}

size_t CLispEng::HashEq(CP p) {
#ifdef X_DEBUG//!!!D
	if (Type(p) == TS_OBJECT)
		p = p;
#endif
	return AsIndex(p);
}

size_t CLispEng::HashEql(CP p) {
	switch (Type(p)) {
	case TS_RATIO:
	case TS_COMPLEX:
		{
			CConsValue *cons = AsCons(p);
			return HashEql(cons->m_car) ^ HashEql(cons->m_cdr);
		}
	case TS_BIGNUM:
		return hash<BigInteger>()(AsBignum(p)->m_int);
	case TS_FLONUM:
		return hash<double>()(AsFloat(p)->m_d);
	}
	return HashEq(p);
}

static const byte s_depth2rot[4] = { 16, 7, 5, 3 };

size_t CLispEng::HashcodeCons(CP p, int depth) {
	switch (Type(p)) {
	case TS_CONS:
		if (p)
			return depth==4 ? 1
			: RotlSizeT(HashcodeCons(AsCons(p)->m_car, depth+1), s_depth2rot[depth])
			^ HashcodeCons(AsCons(p)->m_cdr, depth+1);
	default:
		return HashEql(p);
	}
}

size_t CLispEng::HashEqual(CP p)  {			// also for EQUALP
	switch (Type(p)) {
	case TS_ARRAY:
		{
			size_t hv = 0;
			CArrayValue *av = AsArray(p);
			for (size_t i=av->TotalSize(); i--;)
				hv += HashEql(av->GetElement(i));
			return hv;
		}	
	case TS_PATHNAME:
		{
			CPathname *pn = AsPathname(p);
			size_t hv = HashEqual(pn->m_host)+HashEqual(pn->m_dir)+HashEqual(pn->m_name)+HashEqual(pn->m_type)+HashEqual(pn->m_ver);
			if (!pn->LogicalP)
				hv += HashEqual(pn->m_dev);
			return hv;
		}
	default:
		return HashcodeCons(p);
	}
}

size_t CLispEng::SxHash(CP p) {
	return HashEqual(p);
}

void CLispEng::F_SxHash() {
	m_r = CreateFixnum(SxHash(Pop()) & (FIXNUM_LIMIT-1));
}

void CLispEng::F_MakeHashTable() {
	SkipStack(4);
	CHashTable *ht = CreateHashTable();
	ht->m_rehashThreshold = ToOptional(SV1, V_1);
	ht->m_rehashSize = ToOptional(SV2, V_1);
	CHashMap& hm = *ht->m_pMap;
	hm.SetTestFun(ToOptional(SV4, S(L_EQL)));

	for (CP inits=ToOptionalNIL(SV), c; SplitPair(inits, c);) {
		CConsValue *cons = AsCons(c);
		hm.insert(make_pair(cons->m_car, cons->m_cdr));
	}

	m_r = FromSValue(ht);
	SkipStack(5);
}

// For 1 < n <= 16,
//   (hash-tuple-function n ...) =
//   (cons (hash-tuple-function n1 ...) (hash-tuple-function n2 ...)) */
static const byte
	s_tupleHalf1[17] = { 0, 0, 1, 1, 2, 2, 2, 3, 4, 4, 4, 4, 4, 5, 6, 7, 8 },
	s_tupleHalf2[17] = { 0, 0, 1, 2, 2, 3, 4, 4, 4, 5, 6, 7, 8, 8, 8, 8, 8 };


bool CLispEng::EqualTuple(CP p, size_t n, CP *pRest) {
	if (n == 1)
		return p == pRest[-1];
	else if (!ConsP(p))
		return false;
	else if (n <= 16) {
		size_t n1 = s_tupleHalf1[n];
		return EqualTuple(Car(p), n1, pRest) && EqualTuple(Cdr(p), s_tupleHalf2[n], pRest-n1);
	}
	if (!EqualTuple(Car(p), 8, pRest))
		return false;
	Inc(p);
	if (!ConsP(p) || !EqualTuple(Car(p), 4, pRest-8))
		return false;
	Inc(p);
	if (!ConsP(p) || !EqualTuple(Car(p), 2, pRest-12))
		return false;
	Inc(p);
	n -= 14;
	pRest -= 14;
	for (; n--; Inc(p), pRest--)
		if (!ConsP(p) || Car(p)!=pRest[-1])
			return false;
	return !p;	
}

bool CLispEng::EqualTuple(CP p, CP q) {
	if (Type(p) == TS_STACK_RANGE)
		swap(p, q);
	pair<int, int> pp = FromStackRange(q);
	return EqualTuple(p, pp.second, m_pStackBase+pp.first);
}

size_t CLispEng::HashcodeTuple(size_t n, CP *pRest, int depth) {
	if (n == 1)
		return HashEq(pRest[-1]); //!!! (size_t)AsFixnum( TheClass(pRest[-1]).Hashcode);
	else if (n <= 16) {
		size_t n1 = s_tupleHalf1[n];
		return RotlSizeT(HashcodeTuple(n1, pRest, depth+1), s_depth2rot[depth]) ^ HashcodeTuple(s_tupleHalf2[n], pRest-n1, depth+1);
	}
	return RotlSizeT(HashcodeTuple(8, pRest, 1), 16)		^ RotlSizeT(HashcodeTuple(4, pRest-8, 2), 7) ^
		RotlSizeT(HashcodeTuple(2, pRest-12, 3), 5) ^ RotlSizeT(HashcodeTuple(1, pRest-14, 4), 3) ^ 1;
}

void CLispEng::F_ClassTupleGethash(size_t nArgs) {
	CP *pRest = m_pStack + ++nArgs;
	for (int i=0; i<nArgs; i++) {
		Push(pRest[-1-i]);
		F_ClassOf();
		pRest[-1-i] = m_r;
	}
	CHashMap& hm = *ToHashTable(pRest[0])->m_pMap;
	CHashMap::iterator i = hm.find(CreateStackRange(pRest-m_pStackBase, nArgs));
	m_r = i != hm.end() ? i->second : 0;
	m_pStack = pRest+1;
}

} // Lisp::

