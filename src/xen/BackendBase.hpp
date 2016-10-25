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

#ifndef INCLUDE_BACKENDBASE_HPP_
#define INCLUDE_BACKENDBASE_HPP_

#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

extern "C"
{
	#include "xenctrl.h"
}

#include "FrontendHandlerBase.hpp"
#include "XenStore.hpp"
#include "XenStat.hpp"

namespace XenBackend {

class BackendException : public std::exception
{
public:
	BackendException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class BackendBase
{
public:
	BackendBase(int domId, const std::string& deviceName, int id = 0);
	virtual ~BackendBase();

	void run();
	void stop();

	const std::string& getDeviceName() const { std::lock_guard<std::mutex> lock(mMutex); return mDeviceName; }
	const std::string& getXsDomPath() const { std::lock_guard<std::mutex> lock(mMutex); return mXsDomPath; }
	int getId() const { std::lock_guard<std::mutex> lock(mMutex); return mId; }
	int getDomId() const { std::lock_guard<std::mutex> lock(mMutex); return mDomId; }
	XenStore& getXenStore() { std::lock_guard<std::mutex> lock(mMutex); return mXenStore; }
	xc_gnttab* getXcGntTab() { std::lock_guard<std::mutex> lock(mMutex); return mXcGnttab; }

protected:
	virtual bool getNewFrontend(int& domId, int& id);
	virtual void onNewFrontend(int domId, int id) = 0;

	void addFrontendHandler(const std::shared_ptr<FrontendHandlerBase> frontendHandler);

private:
	int cPollFrontendIntervalMs = 100;

	int mId;
	int mDomId;
	std::string mDeviceName;
	std::string mXsDomPath;
	XenStore mXenStore;
	XenStat mXenStat;
	xc_gnttab* mXcGnttab;

	mutable std::mutex mMutex;

	std::map<std::pair<int, int>, std::shared_ptr<FrontendHandlerBase>> mFrontendHandlers;


	std::atomic_bool mTerminate;

	void initXen();
	void releaseXen();
	void createFrontendHandler(const std::pair<int, int>& ids);
	void checkTerminatedFrontends();
};

}

#endif /* INCLUDE_BACKENDBASE_HPP_ */
