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
using std::mutex;
using std::stoi;
using std::string;
using std::stringstream;
using std::thread;
using std::to_string;
using std::vector;

FrontendHandlerBase::FrontendHandlerBase(int domId, BackendBase& backend, XenStore& xenStore) :
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
	stop();

	LOG(INFO) << "Delete frontend handler: " << mDomId;
}

void FrontendHandlerBase::start()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Start frontend handler: " << mDomId;

	setXsWatches();

	mThread = thread(&FrontendHandlerBase::run, this);
}

void FrontendHandlerBase::stop()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Stop frontend handler: " << mDomId;

	clearXsWatches();

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

void FrontendHandlerBase::onBind()
{
	LOG(INFO) << "On bind frontend handler: " << mDomId;
}

void FrontendHandlerBase::bindChannel(const string& name)
{

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

		}
	}
	catch(const FrontendHandlerException& e)
	{
		LOG(ERROR) << e.what();
	}

	clearXsWatches();
}

void FrontendHandlerBase::initXsPathes()
{
	mXsDomPath = mXenStore.getDomainPath(mDomId);

	stringstream ss;

	ss << mXsDomPath << "/device/" << mBackend.getDeviceName() << "/" << mBackend.getId();

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
	LOG(INFO) << "Wait for backend initialized: " << mDomId;

	auto state = waitForState(mXsBackendPath, {XenbusStateInitialising, XenbusStateClosed,
											   XenbusStateInitWait, XenbusStateConnected,
											   XenbusStateClosing});

	if (state == XenbusStateInitWait || state == XenbusStateConnected)
	{
		LOG(ERROR) << "Fudging state to " << XenbusStateInitialising;
	}
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

	waitForState(mXsFrontendPath, {XenbusStateConnected});
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

		while(!mTerminate && !mXenStore.checkWatches())
		{
		}

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

void FrontendHandlerBase::setXsWatches()
{
	LOG(INFO) << "Set XS watches: " << mDomId;

	mXenStore.setWatch(mXsBackendPath);
	mXenStore.setWatch(mXsFrontendPath);
}

void FrontendHandlerBase::clearXsWatches()
{
	LOG(INFO) << "Clear XS watches: " << mDomId;

	mXenStore.clearWatch(mXsBackendPath);
	mXenStore.clearWatch(mXsFrontendPath);
}
