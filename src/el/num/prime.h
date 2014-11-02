#pragma once

#include <el/bignum.h>

namespace Ext { namespace Num {

const size_t MAX_PRIME_FACTOR = 5000,
	PRIME_LIMIT = MAX_PRIME_FACTOR * MAX_PRIME_FACTOR;

const vector<UInt32>& PrimeTable();

BigInteger Primorial(UInt32 n);

}} // Ext::Num::



