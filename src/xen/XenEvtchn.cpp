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

#include "XenEvtchn.hpp"

#include <poll.h>

#include <glog/logging.h>

using std::to_string;

namespace XenBackend {

XenEvtchn::XenEvtchn(int domId, int port)
{
	try
	{
		init(domId, port);
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenEvtchn::~XenEvtchn()
{
	release();
}

bool XenEvtchn::waitEvent()
{
	pollfd fds;

	fds.fd = xenevtchn_fd(mHandle);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPoolEventTimeoutMs);

	if (ret > 0)
	{
		auto port = xenevtchn_pending(mHandle);

		if (port < 0)
		{
			throw XenEvtchnException("Can't get pending port");
		}

		if (xenevtchn_unmask(mHandle, port) < 0)
		{
			throw XenEvtchnException("Can't unmask event channel");
		}

		if (port != mPort)
		{
			throw XenEvtchnException("Error port number: " + to_string(port) + ", expected: " + to_string(mPort));

			return false;
		}

		DVLOG(2) << "Event received, port: " << mPort;

		return true;
	}

	if (ret < 0)
	{
		throw XenEvtchnException("Can't poll watches");
	}

	return false;
}

void XenEvtchn::notify()
{
	DVLOG(2) << "Notify event channel, port: " << mPort;

	if (xenevtchn_notify(mHandle, mPort) < 0)
	{
		throw XenEvtchnException("Can't notify event channel");
	}
}

void XenEvtchn::init(int domId, int port)
{
	mHandle = xenevtchn_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenEvtchnException("Can't open event channel");
	}

	mPort = xenevtchn_bind_interdomain(mHandle, domId, port);

	if (mPort == -1)
	{
		throw XenEvtchnException("Can't bind event channel");
	}

	VLOG(1) << "Create event channel, dom: " << domId << ", remote port: " << port << ", local port: " << mPort;
}

void XenEvtchn::release()
{
	if (mPort != -1)
	{
		xenevtchn_unbind(mHandle, mPort);
	}

	if (mHandle)
	{
		xenevtchn_close(mHandle);
	}

	VLOG(1) << "Delete event channel, local port: " << mPort;
}

}
