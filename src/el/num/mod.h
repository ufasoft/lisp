#pragma once

// Modular arithmetic

#include <el/num/num.h>

namespace Ext { namespace Num {

class ModNum {
public:
	BigInteger m_val;
	BigInteger m_mod;

	ModNum(const BigInteger& val, const BigInteger& mod)
		:	m_val(val)
		,	m_mod(mod)
	{}
};

ModNum inverse(const ModNum& mn);
UInt32 inverse(UInt32 x, UInt32 mod);

Bn PowM(const Bn& x, const Bn& e, const Bn& mod);
ModNum pow(const ModNum& mn, const BigInteger& p);


}} // Ext::Num::

