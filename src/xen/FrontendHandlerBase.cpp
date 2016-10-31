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
#include "Utils.hpp"

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

FrontendHandlerBase::FrontendHandlerBase(int domId, BackendBase& backend, int id) :
	mId(id),
	mDomId(domId),
	mBackend(backend),
	mTerminate(false),
	mTerminated(false)
{
	mLogId = Utils::logDomId(mDomId, mId) + " - ";

	VLOG(1) << mLogId << "Create frontend handler";

	initXenStorePathes();

	setBackendState(XenbusStateInitialising);

	mXenStore.setWatch(mXsFrontendPath);
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	mChannels.clear();

	stop();

	setBackendState(XenbusStateClosed);

	mXenStore.clearWatch(mXsFrontendPath);

	VLOG(1) << mLogId << "Delete frontend handler";
}

void FrontendHandlerBase::start()
{
	lock_guard<mutex> lock(mMutex);

	VLOG(1) << mLogId << "Start frontend handler";

	mThread = thread(&FrontendHandlerBase::run, this);
}

void FrontendHandlerBase::stop()
{
	VLOG(1) << mLogId << "Stop frontend handler";

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

void FrontendHandlerBase::addChannel(shared_ptr<DataChannelBase> channel)
{
	lock_guard<mutex> lock(mMutex);

	mChannels.insert(make_pair(channel->getName(), channel));

	channel->start();

	LOG(INFO) << mLogId << "Add channel: " << channel->getName();
}

void FrontendHandlerBase::run()
{
	try
	{

		//waitForBackendInitialized();

		waitForFrontendInitialising();

		setBackendState(XenbusStateInitWait);

		waitForFrontendInitialised();

		onBind();

		setBackendState(XenbusStateConnected);

		waitForFrontendConnected();

		LOG(INFO) << mLogId << "Initialization completed";

		while(!mTerminate)
		{
			monitorFrontendState();

			checkTerminatedChannels();
		}
	}
	catch(const exception& e)
	{
		LOG(ERROR) << e.what();
	}

	setBackendState(XenbusStateClosing);

	mTerminated = true;
}

void FrontendHandlerBase::initXenStorePathes()
{
	stringstream ss;

	ss << mXenStore.getDomainPath(mDomId) << "/device/" << mBackend.getDeviceName() << "/" << mId;

	mXsFrontendPath = ss.str();

	ss.str("");
	ss.clear();

	ss << mXenStore.getDomainPath(mBackend.getId()) << "/backend/" << mBackend.getDeviceName() << "/" <<
			mDomId << "/" << mBackend.getId();

	mXsBackendPath = ss.str();

	VLOG(1) << "Frontend path: " << mXsFrontendPath;
	VLOG(1) << "Backend path:  " << mXsBackendPath;
}

void FrontendHandlerBase::waitForBackendInitialised()
{
	mXenStore.setWatch(mXsBackendPath);

	LOG(INFO) << mLogId << "Wait for backend initialized";

	auto state = waitForState(mXsBackendPath, {XenbusStateInitialising, XenbusStateClosed,
											   XenbusStateInitWait, XenbusStateConnected,
											   XenbusStateClosing});

	if (state == XenbusStateInitWait || state == XenbusStateConnected)
	{
		LOG(ERROR) << "Fudging state to " << XenbusStateInitialising;
	}

	mXenStore.clearWatch(mXsBackendPath);
}

void FrontendHandlerBase::waitForFrontendInitialising()
{
	LOG(INFO) << mLogId << "Wait for frontend initializing";

	auto state = waitForState(mXsFrontendPath, {XenbusStateInitialising});

	LOG(INFO) << mLogId << "Frontend state changed to: " << Utils::logState(state);
}

void FrontendHandlerBase::waitForFrontendInitialised()
{
	LOG(INFO) << mLogId << "Wait for frontend initialized";

	auto state = waitForState(mXsFrontendPath, {XenbusStateInitialised});

	LOG(INFO) << mLogId << "Frontend state changed to: " << Utils::logState(state);
}

void FrontendHandlerBase::waitForFrontendConnected()
{
	LOG(INFO) << mLogId << "Wait for frontend connected";

	auto state = waitForState(mXsFrontendPath, {XenbusStateConnected, XenbusStateInitialising});

	LOG(INFO) << mLogId << "Frontend state changed to: " << Utils::logState(state);

	if (state == XenbusStateInitialising)
	{
		throw FrontendHandlerException(mLogId + "Frontend terminated");
	}
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

void FrontendHandlerBase::checkTerminatedChannels()
{
	lock_guard<mutex> lock(mMutex);

	for (auto it = mChannels.begin(); it != mChannels.end();)
	{
		if (it->second->isTerminated())
		{
			LOG(INFO) << mLogId << "Delete terminated channel: " << it->first;

			it = mChannels.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void FrontendHandlerBase::frontendStateChanged(xenbus_state state)
{
	LOG(INFO) << mLogId << "Frontend state changed to: " << Utils::logState(state);

	if (state != XenbusStateConnected)
	{
		mChannels.clear();

		mTerminate = true;
	}
}

xenbus_state FrontendHandlerBase::waitForState(const string& nodePath, const vector<xenbus_state>& states)
{
	while(true)
	{
		xenbus_state state = static_cast<xenbus_state>(mXenStore.readInt(nodePath + "/state"));

		if (find(states.begin(), states.end(), state) != states.end())
		{
			return state;
		}

		while(!mTerminate && !mXenStore.checkWatches());

		if (mTerminate)
		{
			throw FrontendHandlerException(mLogId + "Frontend terminated");
		}
	}

	return XenbusStateUnknown;
}

void FrontendHandlerBase::setBackendState(xenbus_state state)
{
	LOG(INFO) << mLogId << "Set backend state to: " << Utils::logState(state);

	auto path = mXsBackendPath + "/state";

	mXenStore.writeInt(path, state);
}

}
