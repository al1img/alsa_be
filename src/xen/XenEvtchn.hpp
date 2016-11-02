/*
 *  Xen evtchn wrapper
 *  Copyright (c) 2016, Oleksandr Grytsov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef SRC_XEN_XENEVTCHN_HPP_
#define SRC_XEN_XENEVTCHN_HPP_

extern "C" {
#include <xenevtchn.h>
}

#include "XenException.hpp"

namespace XenBackend {

class XenEvtchnException : public XenException
{
	using XenException::XenException;
};

class XenEvtchn
{
public:
	XenEvtchn(int domId, int port);
	~XenEvtchn();

	bool waitEvent();
	void notify();

private:
	const int cPoolEventTimeoutMs = 100;

	xenevtchn_handle *mHandle;
	int mPort;

	void init(int domId, int port);
	void release();
};

}

#endif /* SRC_XEN_XENEVTCHN_HPP_ */
