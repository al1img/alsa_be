/*
 *  Xen event channel wrapper
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

#include "EventChannel.hpp"

#include <poll.h>

#include <glog/logging.h>

#include "FrontendHandlerBase.hpp"
#include "XenStore.hpp"

using std::string;
using std::to_string;

namespace XenBackend {

EventChannel::EventChannel(FrontendHandlerBase& frontendHandler, const std::string& portPath) :
	mFrontendHandler(frontendHandler),
	mDomId(frontendHandler.getDomId()),
	mPortPath(portPath),
	mXenStore(frontendHandler.getXenStore()),
	mHandle(nullptr),
	mPort(-1)
{
	try
	{
		initXen();

		VLOG(1) << "Create event channel, port: " << mPort << ", dom: " << mDomId;
	}
	catch(const EventChannelException& e)
	{
		releaseXen();

		throw;
	}
}

EventChannel::~EventChannel()
{
	VLOG(1) << "Delete event channel, port: " << mPort << ", dom: " << mDomId;

	releaseXen();
}

bool EventChannel::waitEvent()
{
	pollfd fds;

	fds.fd = xc_evtchn_fd(mHandle);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPoolEventTimeout);

	if (ret > 0)
	{
		auto port = xc_evtchn_pending(mHandle);

		if (port < 0)
		{
			throw EventChannelException("Can't get pending port");
		}

		if (xc_evtchn_unmask(mHandle, port) < 0)
		{
			throw EventChannelException("Can't unmask event channel");
		}

		if (port != mPort)
		{
			throw EventChannelException("Error port number: " + to_string(port) + ", expected: " + to_string(mPort));

			return false;
		}

		DVLOG(2) << "Event received, port: " << mPort << ", dom: " << mDomId;

		return true;
	}

	if (ret < 0)
	{
		throw XenStoreException("Can't poll watches");
	}

	return false;

}

void EventChannel::notify()
{
	DVLOG(2) << "Notify event channel, port: " << mPort << ", dom: " << mDomId;

	if (xc_evtchn_notify(mHandle, mPort) < 0)
	{
		throw EventChannelException("Can't notify event channel");
	}
}

void EventChannel::initXen()
{
	mHandle = xc_evtchn_open(nullptr, 0);

	if (!mHandle)
	{
		throw EventChannelException("Can't open event channel");
	}

	auto port = mXenStore.readInt(mPortPath);

	VLOG(1) << "Read event channel port: " << mPortPath << ", dom: " << mDomId << ", port: " << port;

	mPort = xc_evtchn_bind_interdomain(mHandle, mDomId, port);

	if (mPort == -1)
	{
		throw EventChannelException("Can't bind event channel");
	}

	VLOG(1) << "Bind event channel port: " << ", dom: " << mDomId << ", remote port: " << port << ", local port: " << mPort;
}

void EventChannel::releaseXen()
{
	if (mPort != -1)
	{
		xc_evtchn_unbind(mHandle, mPort);
	}

	if (mHandle)
	{
		xc_evtchn_close(mHandle);
	}
}

}
