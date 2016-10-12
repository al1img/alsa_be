/*
 * FrontendHandlerBase.cpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#include "FrontendHandlerBase.hpp"

#include <sstream>

#include <glog/logging.h>

extern "C"
{
	#include "xenstore.h"
	#include "xenctrl.h"
}

#include "BackendBase.hpp"

using std::stringstream;

FrontendHandlerBase::FrontendHandlerBase(int domId, const BackendBase& backend) :
	mDomId(domId),
	mBackend(backend)
{
	try
	{
		LOG(INFO) << "Create frontend handler: " << mDomId;

		mXsFrontendPath = getXsFrontendPath();
		mXsBackendPath = getXsBackendPath();

		LOG(INFO) << "Frontend Path: " << mXsFrontendPath;
		LOG(INFO) << "Backend Path: " << mXsBackendPath;
	}
	catch(const FrontendHandlerException& e)
	{
		throw;
	}
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	xs_rm(mBackend.getXsHandle(), XBT_NULL, getXsRemovePath().c_str());

	LOG(INFO) << "Delete frontend handler: " << mDomId;
}

const std::string FrontendHandlerBase::getXsBackendPath()
{
	stringstream ss;

	ss << mBackend.getXsDomPath() << "/backend/" << mBackend.getDeviceName() << "/" <<
			mDomId << "/" << mBackend.getId();

	return ss.str();
}

const std::string FrontendHandlerBase::getXsFrontendPath()
{
	auto path = xs_get_domain_path(mBackend.getXsHandle(), mDomId);

	if (!path)
	{
		throw FrontendHandlerException("Can't get domain path");
	}

	stringstream ss;

	ss << path << "/device/" << mBackend.getDeviceName() << "/" << mBackend.getId();

	free(path);

	return ss.str();
}

const std::string FrontendHandlerBase::getXsRemovePath()
{
	stringstream ss;

	ss << mBackend.getXsDomPath() << "/backend/" << mBackend.getDeviceName() << "/" <<
			mDomId;

	return ss.str();
}
