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

EventChannel::EventChannel(FrontendHandlerBase& frontendHandler, const std::string& eventChannelPath) :
	mFrontendHandler(frontendHandler),
	mDomId(frontendHandler.getDomId()),
	mEventChannelPath(eventChannelPath),
	mXenStore(frontendHandler.getXenStore()),
	mEventChannel(nullptr),
	mPort(-1)
{
	try
	{
		LOG(INFO) << "Create event channel: " << mEventChannelPath << ", dom: " << mDomId;

		initXen();
	}
	catch(const EventChannelException& e)
	{
		releaseXen();

		throw;
	}
}

EventChannel::~EventChannel()
{
	LOG(INFO) << "Delete event channel: " << mEventChannelPath << ", dom: " << mDomId;

	releaseXen();
}

bool EventChannel::waitEvent()
{
	pollfd fds;

	fds.fd = xc_evtchn_fd(mEventChannel);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPoolEventTimeout);

	if (ret > 0)
	{
		auto port = xc_evtchn_pending(mEventChannel);

		if (port < 0)
		{
			throw EventChannelException("Can't get pending port");
		}

		if (xc_evtchn_unmask(mEventChannel, port) < 0)
		{
			throw EventChannelException("Can't unmask event channel");
		}

		if (port != mPort)
		{
			throw EventChannelException("Error port number");
		}

		LOG(INFO) << "Event received: " << mEventChannelPath << ", dom: " << mDomId;

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
	LOG(INFO) << "Notify event channel: " << mEventChannelPath << ", dom: " << mDomId;

	if (xc_evtchn_notify(mEventChannel, mPort) < 0)
	{
		throw EventChannelException("Can't notify event channel");
	}
}

void EventChannel::initXen()
{
	mEventChannel = xc_evtchn_open(nullptr, 0);

	if (!mEventChannel)
	{
		throw EventChannelException("Can't open event channel");
	}

	mPort = mXenStore.readInt(mFrontendHandler.getXsFrontendPath() + "/" + mEventChannelPath);

	if (xc_evtchn_bind_interdomain(mEventChannel, mDomId, mPort) == -1)
	{
		mPort = -1;

		throw EventChannelException("Can't bind event channel");
	}
}

void EventChannel::releaseXen()
{
	if (mPort != -1)
	{
		xc_evtchn_unbind(mEventChannel, mPort);
	}

	if (mEventChannel)
	{
		xc_evtchn_close(mEventChannel);
	}
}
