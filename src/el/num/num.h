#pragma once

#ifndef UCFG_NUM
#	define UCFG_NUM 'G'
#endif

#include <el/bignum.h>

#if UCFG_NUM == 'G'
#	pragma warning(push)
#		pragma warning(disable: 4146 4244)
#		include EXT_GMP_HEADER
#	pragma warning(pop)
#else
#	include <el/crypto/ext-openssl.h>
#	include <openssl/bn.h>
#endif

// Modular arithmetic


namespace Ext { namespace Num {

#if UCFG_GMP_IMP=='M'
	typedef mpir_ui mp_ui_t;
#else
	typedef unsigned long mp_ui_t;
#endif


class Bn {
	typedef Bn class_type;
public:
	Bn() {
		::mpz_init(m_z);
	}

	Bn(const Bn& x) {
		::mpz_init_set(m_z, x.m_z);
	}

	Bn(int n) {
		::mpz_init_set_si(m_z, n);
	}

	Bn(long n) {
		::mpz_init_set_si(m_z, n);
	}

	Bn(mp_ui_t n) {
		::mpz_init_set_ui(m_z, n);
	}

	Bn(Bn&& x) {
		*m_z = *x.m_z;
#if UCFG_STDSTL || UCFG_DEBUG
		::mpz_init(x.m_z);
#else
		x.m_z->_mp_d = 0;
		x.m_z->_mp_alloc = x.m_z->_mp_size = 0;
#endif
	}

	Bn(size_t bits, bool) {
		::mpz_init2(m_z, bits);
	}

	explicit Bn(const BigInteger& bn);
	~Bn();

	void swap(Bn& x) {
		mpz_swap(m_z, x.m_z);
	}

	Bn& operator=(const Bn& x) {
		::mpz_set(m_z, x.m_z);		
		return *this;
	}

	Bn& operator=(Bn&& x) {
		swap(x);
		return *this;
	}

	Bn& operator<<=(size_t n);
	Bn operator<<(size_t n) const;

	Bn& operator+=(int n);
	Bn& operator+=(const Bn& x);
	Bn& operator-=(int n);

	Bn operator*(const Bn& y) const {
		Bn r;
		::mpz_mul(r.m_z, m_z, y.m_z);
		return r;
	}

	Bn& operator*=(unsigned long n);

	Bn operator/(mp_ui_t y) const {
		Bn r;
		::mpz_tdiv_q_ui(r.m_z, m_z, y);
		return r;
	}

	mp_ui_t operator%(mp_ui_t d) const {
		return ::mpz_fdiv_ui(m_z, d);
	}

	mpz_srcptr get_mpz_t() const { return m_z; }
	mpz_ptr get_mpz_t() { return m_z; }

	bool operator==(unsigned long n) const {
		return !mpz_cmp_ui(m_z, n);
	}

	bool operator==(const Bn& x) const {
		return !mpz_cmp(m_z, x.m_z);
	}

	size_t get_Length() const {
		return ::mpz_sizeinbase(m_z, 2);
	}
	DEFPROP_GET(size_t, Length);

	mp_ui_t get_ui() const {
		return ::mpz_get_ui(m_z);
	}

	bool DivisibleP(mp_ui_t d) const {
		return ::mpz_divisible_ui_p(m_z, d);
	}

	BigInteger ToBigInteger() const;
	static Bn FromBinary(const ConstBuf& cbuf, Endian endian);
	void ToBinary(byte *p, size_t n) const;

private:
	mpz_t m_z;
};

inline void swap(Bn& x, Bn& y) {
	x.swap(y);
}

ostream& operator<<(ostream& os, const Bn& bn);

Bn operator+(const Bn& x, long n);
Bn operator+(const Bn& x, unsigned long n);
inline Bn operator+(const Bn& x, int n) { return operator+(x, static_cast<long>(n)); }

Bn operator-(const Bn& x, long n);
Bn operator-(const Bn& x, unsigned long n);
Bn operator-(const Bn& x, const Bn& y);
inline Bn operator-(const Bn& x, int n) { return operator-(x, static_cast<long>(n)); }

Bn operator/(const Bn& x, const Bn& y);

int ProbabPrimeP(const Bn& bn, int reps = 25);

}} // Ext::Num::

