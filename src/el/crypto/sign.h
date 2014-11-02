#pragma once 


namespace Ext { namespace Crypto {
using namespace std;

class DsaBase : public Object {
public:
	virtual Blob SignHash(const ConstBuf& hash) =0;
	virtual bool VerifyHash(const ConstBuf& hash, const ConstBuf& signature) =0;
};


}} // Ext::Crypto::

