#include <el/ext.h>

#include "prime.h"

namespace Ext { namespace Num {

class PrimeNumbers : public vector<UInt32> {
	typedef dynamic_bitset<> base;
public:
	PrimeNumbers() {
		ASSERT(MAX_PRIME_FACTOR*MAX_PRIME_FACTOR == PRIME_LIMIT);

		size_t lim = PRIME_LIMIT/2;
		dynamic_bitset<> vComposite(lim );
		for (UInt32 n=3; n<MAX_PRIME_FACTOR; n+=2) {
			if (!vComposite[n/2]) {
				for (UInt32 m=n*n/2; m<lim; m+=n)
					vComposite.set(m);
			}
		}
		push_back(2);
		for (UInt32 p=3; p<PRIME_LIMIT; p+=2)
			if (!vComposite[p/2])
				push_back(p);
	}
};

static InterlockedSingleton<PrimeNumbers> s_primeNumbers;

const vector<UInt32>& PrimeTable() {
	return *s_primeNumbers;
}

BigInteger Primorial(UInt32 n) {
	const vector<UInt32>& primes = PrimeTable();
	BigInteger r = 1;
	EXT_FOR (UInt32 p, primes) {
		if (p > n)
			break;
		r *= BigInteger(p);
	}
	return r;
}

}} // Ext::Num::

