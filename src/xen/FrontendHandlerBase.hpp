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

#ifndef INCLUDE_FRONTENDHANDLERBASE_HPP_
#define INCLUDE_FRONTENDHANDLERBASE_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <xen/io/xenbus.h>
}

#include "DataChannelBase.hpp"
#include "XenException.hpp"
#include "XenStore.hpp"
#include "Log.hpp"

namespace XenBackend {

class BackendBase;
class XenStore;

class FrontendHandlerException : public XenException
{
	using XenException::XenException;
};

class FrontendHandlerBase
{
public:
	FrontendHandlerBase(int domId, BackendBase& backend, int id = 0);
	virtual ~FrontendHandlerBase();

	int getDomId() const { return mDomId; }
	int getId() const {  return mId; }
	const std::string& getXsFrontendPath() const { return mXsFrontendPath; }
	XenStore& getXenStore() {  return mXenStore; }

	xenbus_state getBackendState();

protected:
	virtual void onBind() = 0;

	void addChannel(std::shared_ptr<DataChannelBase> channel);

private:
	int mId;
	int mDomId;
	BackendBase& mBackend;

	xenbus_state mBackendState;
	xenbus_state mFrontendState;

	XenStore mXenStore;

	std::string mXsBackendPath;
	std::string mXsFrontendPath;

	std::map<std::string, std::shared_ptr<DataChannelBase>> mChannels;

	bool mWaitForFrontendInitialising;

	std::string mLogId;

	Log mLog;

	void run();

	void initXenStorePathes();
	void checkTerminatedChannels();
	void frontendPathChanged(const std::string& path);
	void frontendStateChanged(xenbus_state state);
	void onXenStoreError(const std::exception& e);
	void setBackendState(xenbus_state state);
};

}

#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
