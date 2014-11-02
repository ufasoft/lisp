#include <el/ext.h>

#include "num.h"


#pragma comment(lib, EXT_GMP_LIB)

using namespace Ext;

namespace Ext { namespace Num {

Bn::Bn(const BigInteger& bn) {
	::mpz_init(m_z);
	int n = (bn.Length+8)/8;
	byte *p = (byte*)alloca(n);
	bn.ToBytes(p, n);
	mpz_import(m_z, n, -1, 1, -1, 0, p); //!!!Sgn
}

BigInteger Bn::ToBigInteger() const {
	size_t n = (mpz_sizeinbase(m_z, 2) + 7) / 8 ;
	byte *p = (byte*)alloca(n+1);
	memset(p, 0, n+1);
	mpz_export(p, 0, -1, 1, -1, 0, m_z);				//!!!Sgn
	return BigInteger(p, n+1);
}

Bn::~Bn() {
	::mpz_clear(m_z);
}

Bn& Bn::operator<<=(size_t n) {
	::mpz_mul_2exp(m_z, m_z, n);
	return *this;
}

Bn Bn::operator<<(size_t n) const {
	Bn r;
	::mpz_mul_2exp(r.m_z, m_z, n);
	return r;
}

Bn& Bn::operator+=(int n) {
	n>=0 ? mpz_add_ui(m_z, m_z, n) : mpz_sub_ui(m_z, m_z, static_cast<unsigned int>(-n));
	return *this;
}

Bn& Bn::operator+=(const Bn& x) {
	::mpz_add(m_z, m_z, x.m_z);
	return *this;
}

Bn& Bn::operator-=(int n) {
	n>=0 ? mpz_sub_ui(m_z, m_z, n) : mpz_add_ui(m_z, m_z, static_cast<unsigned int>(-n));
	return *this;
}

Bn& Bn::operator*=(unsigned long n) {
	mpz_mul_ui(m_z, m_z, n);
	return *this;
}
Bn Bn::FromBinary(const ConstBuf& cbuf, Endian endian) {
	Bn r;
	mpz_import(r.m_z, cbuf.Size, endian==Endian::Big ? 1 : -1, 1, -1, 0, cbuf.P); //!!!Sgn
	return r;
}

void Bn::ToBinary(byte *p, size_t n) const {
	size_t count = (mpz_sizeinbase(m_z, 2) + 7) / 8;
	byte *buf = (byte*)alloca(count);
	mpz_export(buf, &count, 1, 1, -1, 0, m_z);				//!!!Sgn
	if (n < count)
		Throw(E_INVALIDARG);
	memset(p, 0, n-count);
	memcpy(p+n-count, buf, count);
}

ostream& operator<<(ostream& os, const Bn& bn) {
	int radix = (os.flags() & ios::basefield) == ios::hex ? 16
		:	(os.flags() & ios::basefield) == ios::oct ? 8 : 10;
	return os << ::mpz_get_str((char*)alloca(2 + ::mpz_sizeinbase(bn.get_mpz_t(), radix)), os.flags() & ios::uppercase ? -radix : radix, bn.get_mpz_t());
}

Bn operator+(const Bn& x, long n) {
	if (n < 0)
		return operator-(x, static_cast<unsigned long>(-n));
	Bn r(x);
	::mpz_add_ui(r.get_mpz_t(), r.get_mpz_t(), n);
	return r;
}

Bn operator+(const Bn& x, unsigned long n) {
	Bn r(x);
	::mpz_add_ui(r.get_mpz_t(), r.get_mpz_t(), n);
	return r;
}

Bn operator-(const Bn& x, long n) {
	if (n < 0)
		return operator+(x, static_cast<unsigned long>(-n));
	Bn r(x);
	::mpz_sub_ui(r.get_mpz_t(), r.get_mpz_t(), n);
	return r;
}

Bn operator-(const Bn& x, unsigned long n) {
	Bn r(x);
	::mpz_sub_ui(r.get_mpz_t(), r.get_mpz_t(), n);
	return r;
}

Bn operator-(const Bn& x, const Bn& y) {
	Bn r;
	::mpz_sub(r.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	return r;
}

Bn operator/(const Bn& x, const Bn& y) {
	Bn r;
	::mpz_tdiv_q(r.get_mpz_t(), x.get_mpz_t(), y.get_mpz_t());
	return r;
}

int ProbabPrimeP(const Bn& bn, int reps) {
	return ::mpz_probab_prime_p(bn.get_mpz_t(), reps);
}

}} // Ext::Num::


