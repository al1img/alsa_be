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

#include <glog/logging.h>

using std::make_pair;
using std::unique_ptr;
using std::pair;
using std::shared_ptr;
using std::stoi;
using std::string;
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
	LOG(INFO) << "Create backend: " << deviceName << ", " << id;

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

	LOG(INFO) << "Delete backend: " << mDeviceName << ", " << mId;
}

void BackendBase::run()
{
	LOG(INFO) << "Wait for frontend";

	while(!mTerminate)
	{
		int domId = -1, id = -1;

		if (!getNewFrontend(domId, id))
		{
			continue;
		}

		pair<int, int> newFrontend(make_pair(domId, id));

		if ((domId > 0) && (mFrontendHandlers.find(newFrontend) == mFrontendHandlers.end()))
		{
			LOG(INFO) << "New frontend domId: " << newFrontend.first << ", instance id: " << newFrontend.second;

			try
			{
				onNewFrontend(newFrontend.first, newFrontend.second);
			}
			catch(const FrontendHandlerException& e)
			{
				mFrontendHandlers.erase(newFrontend);

				LOG(ERROR) << e.what();
			}
		}
	}
}

void BackendBase::stop()
{
	mTerminate = true;
}

void BackendBase::addFrontendHandler(const shared_ptr<FrontendHandlerBase> frontendHandler)
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
				if (mXenStore.checkIfExist(basePath + "/" + strInstance + "/state"))
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

}
