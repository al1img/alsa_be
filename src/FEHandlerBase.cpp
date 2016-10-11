/*
 * FEHandlerBase.cpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#include "FEHandlerBase.hpp"

#include <sstream>

#include <glog/logging.h>

extern "C"
{
	#include "xenstore.h"
	#include "xenctrl.h"
}

#include "BEBase.hpp"

using std::stringstream;

FEHandlerBase::FEHandlerBase(int domId, const BEBase& be) :
	mDomId(domId),
	mBe(be)
{
	try
	{
		LOG(INFO) << "Create FE handler: " << mDomId;

		mXsFePath = getXsFePath();
		mXsBePath = getXsBePath();

		LOG(INFO) << "FE Path: " << mXsFePath;
		LOG(INFO) << "BE Path: " << mXsBePath;
	}
	catch(const FEHandlerException& e)
	{
		throw;
	}
}

FEHandlerBase::~FEHandlerBase()
{
	xs_rm(mBe.getXsh(), XBT_NULL, getXsRemovePath().c_str());

	LOG(INFO) << "Delete FE handler: " << mDomId;
}

const std::string FEHandlerBase::getXsBePath()
{
	stringstream ss;

	ss << mBe.getXsDomPath() << "/backend/" << mBe.getDeviceName() << "/" <<
			mDomId << "/" << mBe.getId();

	return ss.str();
}

const std::string FEHandlerBase::getXsFePath()
{
	char* path = xs_get_domain_path(mBe.getXsh(), mDomId);

	if (!path)
	{
		throw FEHandlerException("Can't get domain path");
	}

	stringstream ss;

	ss << path << "/device/" << mBe.getDeviceName() << "/" << mBe.getId();

	free(path);

	return ss.str();
}

const std::string FEHandlerBase::getXsRemovePath()
{
	stringstream ss;

	ss << mBe.getXsDomPath() << "/backend/" << mBe.getDeviceName() << "/" <<
			mDomId;

	return ss.str();
}
