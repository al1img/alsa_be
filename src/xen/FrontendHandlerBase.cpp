/*
 *  Xen base frontend handler
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

#include "FrontendHandlerBase.hpp"

#include <algorithm>
#include <sstream>

#include <glog/logging.h>

extern "C"
{
	#include "xenstore.h"
	#include "xenctrl.h"
}

#include "BackendBase.hpp"

using std::exception;
using std::find;
using std::lock_guard;
using std::make_pair;
using std::mutex;
using std::shared_ptr;
using std::stoi;
using std::string;
using std::stringstream;
using std::thread;
using std::to_string;
using std::vector;

namespace XenBackend {

FrontendHandlerBase::FrontendHandlerBase(int domId, BackendBase& backend, XenStore& xenStore, int id) :
	mId(id),
	mDomId(domId),
	mBackend(backend),
	mXenStore(xenStore),
	mTerminate(false)
{
	try
	{
		LOG(INFO) << "Create frontend handler: " << mDomId;

		initXsPathes();
	}
	catch(const FrontendHandlerException& e)
	{
		throw;
	}
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	mChannels.clear();

	stop();

	mXenStore.clearWatch(mXsFrontendPath);

	LOG(INFO) << "Delete frontend handler: " << mDomId;
}

void FrontendHandlerBase::start()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Start frontend handler: " << mDomId;

	mXenStore.setWatch(mXsFrontendPath);

	mThread = thread(&FrontendHandlerBase::run, this);
}

void FrontendHandlerBase::stop()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Stop frontend handler: " << mDomId;

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

xc_gnttab* FrontendHandlerBase::getXcGnttab() const
{
	return mBackend.getXcGntTab();
}

void FrontendHandlerBase::addChannel(shared_ptr<DataChannelBase> channel)
{
	mChannels.insert(make_pair(channel->getName(), channel));

	channel->start();
}

void FrontendHandlerBase::run()
{
	try
	{
		waitForBackendInitialized();

		setBackendState(XenbusStateInitWait);

		waitForFrontendInitialized();

		onBind();

		setBackendState(XenbusStateConnected);

		waitForFrontendConnected();

		LOG(INFO) << "Initialization completed: " << mDomId;

		while(!mTerminate)
		{
			monitorFrontendState();
		}
	}
	catch(const exception& e)
	{
		LOG(ERROR) << e.what();
	}
}

void FrontendHandlerBase::initXsPathes()
{
	stringstream ss;

	ss << mXenStore.getDomainPath(mDomId) << "/device/" << mBackend.getDeviceName() << "/" << mId;

	mXsFrontendPath = ss.str();

	ss.str("");
	ss.clear();

	ss << mBackend.getXsDomPath() << "/backend/" << mBackend.getDeviceName() << "/" <<
			mDomId << "/" << mBackend.getId();

	mXsBackendPath = ss.str();

	LOG(INFO) << "Frontend path: " << mXsFrontendPath;
	LOG(INFO) << "Backend path:  " << mXsBackendPath;
}

void FrontendHandlerBase::waitForBackendInitialized()
{
	mXenStore.setWatch(mXsBackendPath);

	LOG(INFO) << "Wait for backend initialized: " << mDomId;

	auto state = waitForState(mXsBackendPath, {XenbusStateInitialising, XenbusStateClosed,
											   XenbusStateInitWait, XenbusStateConnected,
											   XenbusStateClosing});

	if (state == XenbusStateInitWait || state == XenbusStateConnected)
	{
		LOG(ERROR) << "Fudging state to " << XenbusStateInitialising;
	}

	mXenStore.clearWatch(mXsBackendPath);
}

void FrontendHandlerBase::waitForFrontendInitialized()
{
	LOG(INFO) << "Wait for frontend initialized: " << mDomId;

	auto state = waitForState(mXsFrontendPath, {XenbusStateInitialised, XenbusStateConnected});

	if (state == XenbusStateConnected)
	{
		LOG(ERROR) << "Fudging state to " << XenbusStateInitialising;
	}
}

void FrontendHandlerBase::waitForFrontendConnected()
{
	LOG(INFO) << "Wait for frontend connected: " << mDomId;

	auto state = waitForState(mXsFrontendPath, {XenbusStateConnected});
}

void FrontendHandlerBase::monitorFrontendState()
{
	while(!mTerminate)
	{
		if (mXenStore.checkWatches())
		{
			frontendStateChanged(static_cast<xenbus_state>(
					mXenStore.readInt(mXsFrontendPath + "/state")));
		}
	}
}

void FrontendHandlerBase::frontendStateChanged(xenbus_state state)
{
	LOG(INFO) << "Wait for frontend state changed - dom " << mDomId << ", state " << state;
}

xenbus_state FrontendHandlerBase::waitForState(const string& nodePath, const vector<xenbus_state>& states)
{
	while(!mTerminate)
	{
		xenbus_state state = static_cast<xenbus_state>(mXenStore.readInt(nodePath + "/state"));

		if (find(states.begin(), states.end(), state) != states.end())
		{
			return state;
		}

		while(!mTerminate && !mXenStore.checkWatches());

#if 0
		// Can't unblock xs_read_watch on close. Above implementation used (xs_check_watch).

		unsigned dummy;

		auto result = xs_read_watch(mBackend.getXsHandle(), &dummy);

		if (!result)
		{
			throw FrontendHandlerException("Can't read ");
		}

		free(result);
#endif

	}
}

void FrontendHandlerBase::setBackendState(xenbus_state state)
{
	LOG(INFO) << "Set backend state to: " << state;

	auto path = mXsBackendPath + "/state";

	mXenStore.writeInt(path, state);
}

}
