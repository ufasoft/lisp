#include <el/ext.h>


#include "mail-address.h"

namespace Ext { namespace Inet {

static regex s_reMailAddress("([A-Z0-9._%+-]+)@([A-Z0-9.-]+\\.[A-Z]{2,4})", regex::icase);

MailAddress::MailAddress(RCString address) {
	cmatch m;
	if (!regex_match(address.c_str(), m, s_reMailAddress))
		Throw(E_INVALIDARG);
	m_user = m[1];
	m_host = m[2];
}


}} // Ext::Inet::

