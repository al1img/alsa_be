/*
 * BEBase.cpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#include "BEBase.hpp"

#include <glog/logging.h>

using std::unique_ptr;
using std::make_pair;

BEBase::BEBase(int domId, const std::string& deviceName, int beId) :
	mDomId(domId),
	mDeviceName(deviceName),
	mXsh(nullptr),
	mXcg(nullptr),
	mId(beId),
	mTerminate(false)
{
	try
	{
		LOG(INFO) << "Create BE: " << deviceName << ", " << beId;

#if 0
		mXcg = xc_gnttab_open(NULL, 0);

		if (!mXcg)
		{
			throw BEException("Can't open xc grant table");
		}

		mXsh = xs_daemon_open();

		if (!mXsh)
		{
			throw BEException("Can't open xs daemon");
		}

		char* domPath = xs_get_domain_path(mXsh, mDomId);

		if (!domPath)
		{
			throw BEException("Can't get domain path");
		}

		mXsDomPath = domPath;

		free(domPath);
#endif
	}
	catch(const BEException& e)
	{
		releaseXen();

		throw;
	}
}

BEBase::~BEBase()
{
	mFEHandlers.clear();

	releaseXen();

	LOG(INFO) << "Delete BE: " << mDeviceName << ", " << mId;
}

void BEBase::run()
{
	while(!mTerminate)
	{
		int newFeId = getNewFEId();

		if (mFEHandlers.find(newFeId) == mFEHandlers.end())
		{
			LOG(INFO) << "New FE: " << newFeId;

			mFEHandlers.insert(make_pair(newFeId, unique_ptr<FEHandlerBase>(
					new FEHandlerBase(newFeId, *this))));
		}
	}
}

void BEBase::stop()
{
	mTerminate = true;
}

void BEBase::releaseXen()
{
	if (mXsh)
	{
		xs_daemon_close(mXsh);
	}

	if (mXcg)
	{
		xc_gnttab_close(mXcg);
	}
}
