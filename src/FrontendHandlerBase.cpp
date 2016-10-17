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

FrontendHandlerBase::FrontendHandlerBase(int domId, const BackendBase& backend) :
	mDomId(domId),
	mBackend(backend),
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

void FrontendHandlerBase::bindChannel(const std::string& name)
{

}

void FrontendHandlerBase::run()
{
	try
	{
		waitForBackendInitialized();

		setState(XenbusStateInitWait);

		waitForFrontendInitialized();

		onBind();

		setState(XenbusStateConnected);

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
	auto domPath = xs_get_domain_path(mBackend.getXsHandle(), mDomId);

	if (!domPath)
	{
		throw FrontendHandlerException("Can't get domain path");
	}

	mXsDomPath = domPath;

	free(domPath);

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

xenbus_state FrontendHandlerBase::getState(const string& nodePath)
{
	auto path = nodePath + "/state";

	unsigned length;

	auto pData = static_cast<char*>(xs_read(mBackend.getXsHandle(), XBT_NULL, path.c_str(), &length));

	if (!pData)
	{
		throw FrontendHandlerException("Can't read state from: " + path);
	}

	string result(pData);

	free(pData);

	return static_cast<xenbus_state>(stoi(result));
}

xenbus_state FrontendHandlerBase::waitForState(const string& nodePath, const vector<xenbus_state>& states)
{
	while(!mTerminate)
	{
		xenbus_state state = getState(nodePath);

		if (find(states.begin(), states.end(), state) != states.end())
		{
			return state;
		}

		while(!mTerminate)
		{
			auto result = xs_check_watch(mBackend.getXsHandle());

			if (result)
			{
				free(result);

				break;
			}
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

void FrontendHandlerBase::setState(xenbus_state state)
{
	LOG(INFO) << "Set backend state to: " << state;

	auto key = mXsBackendPath + "/state";
	auto value = to_string(state);

	if (!xs_write(mBackend.getXsHandle(), XBT_NULL, key.c_str(), value.c_str(), value.length()))
	{
		throw FrontendHandlerException("Can't write state");
	}
}

void FrontendHandlerBase::setXsWatches()
{
	LOG(INFO) << "Set XS watches: " << mDomId;

	if (!xs_watch(mBackend.getXsHandle(), mXsBackendPath.c_str(), ""))
	{
		throw FrontendHandlerException("Can't set xs watch for backend");
	}

	if (!xs_watch(mBackend.getXsHandle(), mXsFrontendPath.c_str(), ""))
	{
		throw FrontendHandlerException("Can't set xs watch for frontend");
	}
}

void FrontendHandlerBase::clearXsWatches()
{
	LOG(INFO) << "Clear XS watches: " << mDomId;

	xs_unwatch(mBackend.getXsHandle(), mXsBackendPath.c_str(), "");

	xs_unwatch(mBackend.getXsHandle(), mXsFrontendPath.c_str(), "");
}
