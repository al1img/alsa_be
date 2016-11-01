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
	mBackendState(XenbusStateUnknown),
	mTerminate(false),
	mTerminated(false)
{
	mLogId = Utils::logDomId(mDomId, mId) + " - ";

	VLOG(1) << mLogId << "Create frontend handler";

	initXenStorePathes();

	setBackendState(XenbusStateInitialising);
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	mChannels.clear();

	stop();

	setBackendState(XenbusStateClosed);

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
		mXenStore.setWatch(mXsFrontendPath);

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

	mXenStore.clearWatch(mXsFrontendPath);

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

	while(!mTerminate)
	{
		string path, token;

		if (mXenStore.checkWatches(path, token))
		{
			if (static_cast<xenbus_state>(mXenStore.readInt(mXsFrontendPath + "/state")) == XenbusStateInitialising)
			{
				break;
			}
		}
	}

	mXenStore.clearWatch(mXsBackendPath);
}

void FrontendHandlerBase::monitorFrontendState()
{
	while(!mTerminate)
	{
		string path, token;

		if (mXenStore.checkWatches(path, token))
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
	static xenbus_state prevState = XenbusStateUnknown;

	if (state == prevState)
	{
		return;
	}

	prevState = state;

	LOG(INFO) << mLogId << "Frontend state changed to: " << Utils::logState(state);

	switch(state)
	{
	case XenbusStateInitialising:

		if (mBackendState != XenbusStateInitialising)
		{
			LOG(WARNING) << mLogId << "Frontend restarted";

			mTerminate = true;
		}
		else
		{
			setBackendState(XenbusStateInitWait);
		}

		break;

	case XenbusStateInitialised:

		onBind();

		setBackendState(XenbusStateConnected);

		break;

	case XenbusStateClosing:
	case XenbusStateClosed:

		mTerminate = true;

		break;

	default:
		break;
	}
}

void FrontendHandlerBase::setBackendState(xenbus_state state)
{
	LOG(INFO) << mLogId << "Set backend state to: " << Utils::logState(state);

	auto path = mXsBackendPath + "/state";

	mBackendState = state;

	mXenStore.writeInt(path, state);
}

}
