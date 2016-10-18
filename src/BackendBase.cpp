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
using std::string;

BackendBase::BackendBase(int domId, const string& deviceName, int id) try :
	mDomId(domId),
	mDeviceName(deviceName),
	mXcGnttab(nullptr),
	mId(id),
	mTerminate(false),
	mXenStore()
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
	while(!mTerminate)
	{
		auto newFrontendId = getNewFrontendId();

		if (mFrontendHandlers.find(newFrontendId) == mFrontendHandlers.end())
		{
			LOG(INFO) << "New frontend: " << newFrontendId;

			try
			{
				mFrontendHandlers.insert(
						make_pair(newFrontendId, unique_ptr<FrontendHandlerBase>(
						new FrontendHandlerBase(newFrontendId, *this, mXenStore))));

				mFrontendHandlers[newFrontendId]->start();
			}
			catch(const FrontendHandlerException& e)
			{
				mFrontendHandlers.erase(newFrontendId);

				LOG(ERROR) << e.what();
			}
		}
	}
}

void BackendBase::stop()
{
	mTerminate = true;
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
