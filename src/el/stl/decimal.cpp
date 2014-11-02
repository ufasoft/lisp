#include <el/ext.h>
#include <el/bignum.h>

#include "decimal"

namespace ExtSTL {
namespace decimal {
using namespace std;
using namespace Ext;

long long decimal_to_long_long(const decimal64& v) {
	Int64 maxPrev = numeric_limits<long long>::max()/10;
	Int64 r = v.m_val;
	if (v.m_exp >= 0) {
		for (int i=0; i<v.m_exp; ++i) {
			if (std::abs(r) > maxPrev )
				Throw(E_INVALIDARG);
			r *= 10;
		}
	} else
		for (int i=v.m_exp; i<0; ++i)
			r /= 10;
	return r;
}

static const double _M_LN10 = log(10.);


// Algorithm FPP2 by
// Steele & White - How to Print Floating-Point Numbers Accurately. PLDI 1999
decimal64_::decimal64_(double v)
	:	m_val(0)
	,	m_exp(0)
{
	static_assert(numeric_limits<double>::radix==2 && numeric_limits<double>::digits<64, "base 2 FP required with 64-bit doubles required");

	switch(fpclassify(v)) {
	case FP_ZERO:
		return;
	case FP_NAN:
	case FP_INFINITE:
		Throw(E_INVALIDARG);
	}
	double va = fabs(v);
	int p = numeric_limits<double>::digits, e;
	Int64 f = (Int64)ldexp(frexp(va, &e), p);
	int offset = f==(1LL << (p-1));
	BigInteger r = BigInteger(f) << offset+max(0, e-p),
		s = BigInteger(1) << offset+max(0, p-e),
		ml = BigInteger(1) << max(0, e-p),
		mh = ml << offset;
	for (BigInteger nr; (nr=r*10)<s; --m_exp, ml*=10, mh*=10)
		r = nr;
	ASSERT(m_exp == min(0, 1+(int)floor(log10(va))));
	for (; (r<<1)+mh >= (s<<1); ++m_exp, s*=10)
		;
	ASSERT(UCFG_USE_OLD_MSVCRTDLL || m_exp == 1+(int)floor(log10( (va + nextafter(va, numeric_limits<double>::max())) / 2)));
	bool low, high;
	BigInteger dr;
	do {
		--m_exp;
		ml *= 10;
		mh *= 10;
		pair<BigInteger, BigInteger> ur = div(r*10, s);
		ASSERT(ur.first < 10);
		m_val = m_val*10 + explicit_cast<int>(ur.first);
		dr = exchange(r, ur.second) << 1;
		low = dr < ml;
		high = dr > (s<<1)-mh;
	} while (!low && !high);
	m_val += (int)(high && (!low || dr>=s));
	if (v < 0)
		m_val = -m_val;	
}

double decimal_to_double(const decimal64_& v) {
	return double(v.m_val) * exp(v.m_exp * _M_LN10);
}

decimal64 make_decimal64(Int64 val, int exp) {
	decimal64 r;
	r.m_val = val;
	r.m_exp = exp;
	return r;
}

EXT_API ostream& operator<<(ostream& os, const decimal64& v) {
	if (v.m_exp >= 0) {
		os << v.m_val << string(v.m_exp, '0');
	} else {
		int nexp = -v.m_exp;
		Int64 val = v.m_val;
		for (lldiv_t qr; nexp>0 && (qr=div(val, 10LL)).rem == 0; --nexp)
			val = qr.quot;
		Int64 d = 1;
		for (int i=0; i<nexp; ++i)
			d *= 10;
		os << val/d << use_facet<numpunct<char>>(os.getloc()).decimal_point() << setw(nexp) << setfill('0') << val % d;
	}
	return os;
}

static regex s_reDecimal("([-+])?(\\d*)(\\.(\\d*))?([eE](([-+])?(\\d+)))?");

EXT_API istream& operator>>(istream& is, decimal64_& v) {		//!!! TODO read exponent
	string s;
	is >> s;
	smatch m;
	if (regex_match(s, m, s_reDecimal)) {
		string sign = m[1].str(),
				intpart = m[2].str(),
				fppart = m[4].str();

		v = make_decimal64(stoll(sign+intpart+fppart), -(int)fppart.size());
		if (m[5].matched) {
			v.m_exp += atoi(m[6].str().c_str());
		}
	} else
		is.setstate(ios::failbit);
	return is;
}

BigInteger Pow10ToInteger(Int64 a, int exp10) {
	BigInteger r(a);
	for (int i=0; i<exp10; ++i)
		r *= 10;
	return r;
}

decimal64 operator-(const decimal64& a, const decimal64& b) {
	BigInteger na = Pow10ToInteger(a.m_val, max(0, a.m_exp-b.m_exp)),
		nb = Pow10ToInteger(b.m_val, max(0, b.m_exp-a.m_exp));
	return make_decimal64(explicit_cast<Int64>(na-nb), min(a.m_exp, b.m_exp));
}

decimal64 operator*(const decimal64& a, const decimal64& b) {
	BigInteger bi = BigInteger(a.m_val) * BigInteger(b.m_val);
	int off = 0;
	while (bi.Length >= 63) {
		bi /= 10;
		++off;
	}
	return make_decimal64(explicit_cast<Int64>(bi), a.m_exp+b.m_exp+off);
}

static const UInt64 MAX_DECIMAL_M = 10000000000000000000ULL,
					MAX_DECIMAL_M_DIV_10 = MAX_DECIMAL_M/10;

NormalizedDecimal AFXAPI Normalize(const decimal64& a) {
	NormalizedDecimal r = { UInt64(a.m_val < 0 ? -a.m_val : a.m_val), a.m_exp, a.m_val < 0 };
	if (r.M == 0)
		r.Exp = INT_MIN;
	else if (r.M >= MAX_DECIMAL_M) {
		r.M /= 10;
		++r.Exp;
	} else
		for (; r.M < MAX_DECIMAL_M_DIV_10; --r.Exp)
			r.M *= 10;
	return r;
};

bool operator==(const decimal64& a, const decimal64& b) {
	NormalizedDecimal na = Normalize(a),
						nb = Normalize(b);
	return na.Minus==nb.Minus && na.M==nb.M && na.Exp==nb.Exp;
}

bool operator<(const decimal64& a, const decimal64& b) {
	NormalizedDecimal na = Normalize(a),
						nb = Normalize(b);
	if (na.Minus != nb.Minus)
		return na.Minus;
	if (na.Exp != nb.Exp)
		return (na.Exp < nb.Exp) ^ na.Minus;
	return (na.M < nb.M) ^ na.Minus;
}

} // decimal::
}  // ExtSTL::

