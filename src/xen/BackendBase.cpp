/*
 *  Xen backend base class
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

#include "BackendBase.hpp"

#include <chrono>
#include <thread>

#include <glog/logging.h>

#include "Utils.hpp"

using std::chrono::milliseconds;
using std::make_pair;
using std::unique_ptr;
using std::pair;
using std::shared_ptr;
using std::stoi;
using std::string;
using std::this_thread::sleep_for;
using std::vector;

namespace XenBackend {

BackendBase::BackendBase(int domId, const string& deviceName, int id) try :
	mId(id),
	mDomId(domId),
	mDeviceName(deviceName),
	mXcGnttab(nullptr),
	mTerminate(false),
	mXenStore(),
	mXenStat()
{
	VLOG(1) << "Create backend: " << deviceName << ", " << id;

	initXen();
}
catch(const BackendException& e)
{
	releaseXen();

	throw;
}

BackendBase::~BackendBase()
{
	mFrontendHandlers.clear();

	releaseXen();

	VLOG(1) << "Delete backend: " << mDeviceName << ", " << mId;
}

void BackendBase::run()
{
	LOG(INFO) << "Wait for frontend";

	while(!mTerminate)
	{
		int domId = -1, id = -1;

		if (getNewFrontend(domId, id))
		{
			createFrontendHandler(make_pair(domId, id));
		}

		checkTerminatedFrontends();

		sleep_for(milliseconds(cPollFrontendIntervalMs));
	}
}

void BackendBase::stop()
{
	mTerminate = true;
}

void BackendBase::addFrontendHandler(shared_ptr<FrontendHandlerBase> frontendHandler)
{
	pair<int, int> ids(make_pair(frontendHandler->getDomId(), frontendHandler->getId()));

	mFrontendHandlers.insert(make_pair(ids, frontendHandler));

	mFrontendHandlers[ids]->start();
}

bool BackendBase::getNewFrontend(int& domId, int& id)
{
	for (auto dom : mXenStat.getRunningDoms())
	{
		if (dom == mDomId)
		{
			continue;
		}

		string basePath = mXenStore.getDomainPath(dom) + "/device/" + mDeviceName;

		for (string strInstance : mXenStore.readDirectory(basePath))
		{
			auto instance = stoi(strInstance);

			if (mFrontendHandlers.find(make_pair(dom, instance)) == mFrontendHandlers.end())
			{
				string statePath = basePath + "/" + strInstance + "/state";

				if (mXenStore.checkIfExist(statePath))
				{
					domId = dom;
					id = instance;

					return true;
				}
			}
		}
	}

	return false;
}

void BackendBase::initXen()
{
	mXcGnttab = xc_gnttab_open(nullptr, 0);

	if (!mXcGnttab)
	{
		throw BackendException("Can't open xc grant table");
	}

	mXsDomPath = mXenStore.getDomainPath(mDomId);
}

void BackendBase::releaseXen()
{
	if (mXcGnttab)
	{
		xc_gnttab_close(mXcGnttab);
	}
}

void BackendBase::createFrontendHandler(const std::pair<int, int>& ids)
{
	if ((ids.first > 0) && (mFrontendHandlers.find(ids) == mFrontendHandlers.end()))
	{
		LOG(INFO) << "Create new frontend: " << Utils::logDomId(ids.first, ids.second);

		try
		{
			onNewFrontend(ids.first, ids.second);
		}
		catch(const FrontendHandlerException& e)
		{
			mFrontendHandlers.erase(ids);

			LOG(ERROR) << e.what();
		}
	}
}

void BackendBase::checkTerminatedFrontends()
{
	for (auto it = mFrontendHandlers.begin(); it != mFrontendHandlers.end();)
	{
		if (it->second->isTerminated())
		{
			it = mFrontendHandlers.erase(it);

			LOG(INFO) << "Delete terminated frontend: " << Utils::logDomId(it->first.first, it->first.second);
		}
		else
		{
			++it;
		}
	}
}

}
