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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "FrontendHandlerBase.hpp"
#include "XenException.hpp"
#include "XenStore.hpp"
#include "XenStat.hpp"

namespace XenBackend {

class BackendException : public XenException
{
	using XenException::XenException;
};

class BackendBase
{
public:
	BackendBase(int domId, const std::string& deviceName, int id = 0);
	virtual ~BackendBase();

	void run();
	void stop();

	const std::string& getDeviceName() const { std::lock_guard<std::mutex> lock(mMutex); return mDeviceName; }
	int getId() const { std::lock_guard<std::mutex> lock(mMutex); return mId; }
	int getDomId() const { std::lock_guard<std::mutex> lock(mMutex); return mDomId; }

protected:
	virtual bool getNewFrontend(int& domId, int& id);
	virtual void onNewFrontend(int domId, int id) = 0;

	void addFrontendHandler(std::shared_ptr<FrontendHandlerBase> frontendHandler);

private:
	int cPollFrontendIntervalMs = 100;

	int mId;
	int mDomId;
	std::string mDeviceName;
	XenStore mXenStore;
	XenStat mXenStat;

	mutable std::mutex mMutex;

	std::map<std::pair<int, int>, std::shared_ptr<FrontendHandlerBase>> mFrontendHandlers;


	std::atomic_bool mTerminate;

	void createFrontendHandler(const std::pair<int, int>& ids);
	void checkTerminatedFrontends();
};

}

#endif /* INCLUDE_BACKENDBASE_HPP_ */
