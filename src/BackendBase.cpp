/*
 * BackendBase.cpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#include "BackendBase.hpp"

#include <glog/logging.h>

using std::unique_ptr;
using std::make_pair;

BackendBase::BackendBase(int domId, const std::string& deviceName, int id) :
	mDomId(domId),
	mDeviceName(deviceName),
	mXsHandle(nullptr),
	mXcGnttab(nullptr),
	mId(id),
	mTerminate(false)
{
	try
	{
		LOG(INFO) << "Create BE: " << deviceName << ", " << id;

#if 0
		initXen();
#endif
	}
	catch(const BackendException& e)
	{
		releaseXen();

		throw;
	}
}

BackendBase::~BackendBase()
{
	mFrontendHandlers.clear();

	releaseXen();

	LOG(INFO) << "Delete BE: " << mDeviceName << ", " << mId;
}

void BackendBase::run()
{
	while(!mTerminate)
	{
		auto newFrontendId = getNewFrontendId();

		if (mFrontendHandlers.find(newFrontendId) == mFrontendHandlers.end())
		{
			LOG(INFO) << "New FE: " << newFrontendId;

			try
			{
				mFrontendHandlers.insert(
						make_pair(newFrontendId, unique_ptr<FrontendHandlerBase>(
						new FrontendHandlerBase(newFrontendId, *this))));

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
	mXcGnttab = xc_gnttab_open(NULL, 0);

	if (!mXcGnttab)
	{
		throw BackendException("Can't open xc grant table");
	}

	mXsHandle = xs_daemon_open();

	if (!mXsHandle)
	{
		throw BackendException("Can't open xs daemon");
	}

	char* domPath = xs_get_domain_path(mXsHandle, mDomId);

	if (!domPath)
	{
		throw BackendException("Can't get domain path");
	}

	mXsDomPath = domPath;

	free(domPath);

}

void BackendBase::releaseXen()
{
	if (mXsHandle)
	{
		xs_daemon_close(mXsHandle);
	}

	if (mXcGnttab)
	{
		xc_gnttab_close(mXcGnttab);
	}
}
