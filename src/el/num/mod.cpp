#include <el/ext.h>

#include "mod.h"


using namespace Ext;

namespace Ext { namespace Num {

ModNum inverse(const ModNum& mn) {
	Bn v(mn.m_val),
		mod(mn.m_mod);
#if UCFG_NUM == 'G'
	Bn r;
	int rc = mpz_invert(r.get_mpz_t(), v.get_mpz_t(), mod.get_mpz_t());
	ASSERT(rc);
	return ModNum(r.ToBigInteger(), mn.m_mod);
#else
	Bn r;
	BnCtx ctx;
	SslCheck(::BN_mod_inverse(r.Ref(), v, mod, ctx));
	return ModNum(r.ToBigInteger(), mn.m_mod);
#endif
}

UInt32 inverse(UInt32 a, UInt32 nPrime) {
	if (a == 2)
		return (nPrime+1)/2;
    
    int inverse;
    for (int rem0=nPrime, rem1=a%nPrime, rem2, aux0 = 0, aux1 = 1, aux2, quotient; ;) {
        if (rem1 <= 1) {
            inverse = aux1;
            break;
        }

        rem2 = rem0 % rem1;
        quotient = rem0 / rem1;
        aux2 = -quotient * aux1 + aux0;

        if (rem2 <= 1) {
            inverse = aux2;
            break;
        }

        rem0 = rem1 % rem2;
        quotient = rem1 / rem2;
        aux0 = -quotient * aux2 + aux1;

        if (rem0 <= 1) {
            inverse = aux0;
            break;
        }

        rem1 = rem2 % rem0;
        quotient = rem2 / rem0;
        aux1 = -quotient * aux0 + aux2;
    }
	ASSERT(UInt64(inverse + nPrime) * a % nPrime == 1);
    return (inverse + nPrime) % nPrime;
}

#if UCFG_NUM == 'G'
Bn PowM(const Bn& x, const Bn& e, const Bn& mod) {
	Bn r(mod.Length+1, false);
	::mpz_powm(r.get_mpz_t(), x.get_mpz_t(), e.get_mpz_t(), mod.get_mpz_t());
	return r;
}
#endif

ModNum pow(const ModNum& mn, const BigInteger& p) {
	Bn v(mn.m_val),
		mod(mn.m_mod);
#if UCFG_NUM == 'G'
	return ModNum(PowM(v, Bn(p), mod).ToBigInteger(), mn.m_mod);
#else
	Bn r;
	BnCtx ctx;
	SslCheck(::BN_mod_exp(r.Ref(), v, Bn(p), mod, ctx));
	return ModNum(r.ToBigInteger(), mn.m_mod);
#endif
}


}} // Ext::Num::


