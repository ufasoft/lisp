/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#pragma once

template <class E, class B> class CGC : public B {
public:
	typedef deque<pair<E, E> > CFreachable;
	CFreachable m_freachable;
protected:
	void CollectFreachable() {
		for (typename CFreachable::iterator i=m_freachable.begin(); i!=m_freachable.end(); ++i) {
			B::ApplyCheck(i->first);
			B::ApplyCheck(i->second);
		}
	}

	void AddToFreachable(pair<E, E> pf) {
		m_freachable.push_back(pf);
	}

	void RunFinalizers()  {//!!! need check after rucursive
		while (!m_freachable.empty()) {
			pair<E, E> pf = m_freachable.front();
			CallFinalizer(pf.second, pf.first);
			m_freachable.pop_front();
		}
	}
};


