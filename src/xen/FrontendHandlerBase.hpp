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
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C"
{
	#include <xen/io/xenbus.h>
	#include <xenctrl.h>
}

#include "DataChannelBase.hpp"

namespace XenBackend {

class BackendBase;
class XenStore;

class FrontendHandlerException : public std::exception
{
public:
	FrontendHandlerException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class FrontendHandlerBase
{
public:
	FrontendHandlerBase(int domId, BackendBase& backend, XenStore& xenStore, int id = 0);
	virtual ~FrontendHandlerBase();

	void start();
	void stop();

	int getDomId() const { return mDomId; }
	int getId() const { return mId; }
	const std::string& getXsFrontendPath() const { return mXsFrontendPath; }
	XenStore& getXenStore() { return mXenStore; }
	xc_gnttab* getXcGnttab() const;

protected:
	virtual void onBind() = 0;

	void addChannel(std::shared_ptr<DataChannelBase> channel);

private:
	int mId;
	int mDomId;
	BackendBase& mBackend;
	XenStore& mXenStore;

	std::string mXsBackendPath;
	std::string mXsFrontendPath;

	std::map<std::string, std::shared_ptr<DataChannelBase>> mChannels;

	std::thread mThread;
	std::mutex mMutex;
	std::atomic_bool mTerminate;

	void run();

	void initXsPathes();
	void waitForBackendInitialized();
	void waitForFrontendInitialized();
	void waitForFrontendConnected();
	void monitorFrontendState();
	void frontendStateChanged(xenbus_state state);

	xenbus_state waitForState(const std::string& nodePath, const std::vector<xenbus_state>& states);

	void setBackendState(xenbus_state state);
};

}

#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
