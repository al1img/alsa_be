/*
 *  Xen Store wrapper
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

#ifndef INCLUDE_XENSTORE_HPP_
#define INCLUDE_XENSTORE_HPP_

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <xenstore.h>
}

#include "XenException.hpp"

namespace XenBackend {

class XenStoreException : public XenException
{
	using XenException::XenException;
};

class XenStore
{
public:
	typedef std::function<void(const std::string& path)> WatchCallback;
	typedef std::function<void(const std::exception&)> WatchErrorCallback;

	XenStore();
	XenStore(const XenStore&) = delete;
	XenStore& operator=(XenStore const&) = delete;
	~XenStore();

	std::string getDomainPath(int domId);
	int readInt(const std::string& path);
	std::string readString(const std::string& path);
	void writeInt(const std::string& path, int value);
	void removePath(const std::string& path);
	bool checkIfExist(const std::string& path);
	std::vector<std::string> readDirectory(const std::string& path);

	void setWatch(const std::string& path, WatchCallback callback, bool initNotify = false);
	void clearWatch(const std::string& path);
	void setWatchErrorCallback(WatchErrorCallback errorCallback);

private:
	const int cPollWatchesTimeoutMs = 100;

	std::function<void(const std::exception&)> mErrorCallback;

	xs_handle*	mXsHandle;

	std::map<std::string, WatchCallback> mWatches;
	std::list<std::string> mInitNotifyWatches;

	std::thread mThread;
	std::mutex mMutex;
	std::mutex mItfMutex;
	bool mCheckWatchResult;

	void init();
	void release();

	void watchesThread();
	bool isWatchesEmpty();
	std::string checkWatches();
	std::string checkXsWatch();
	bool pollXsWatchFd();
	std::string getInitNotifyPath();
	WatchCallback getWatchCallback(std::string& path);
	WatchErrorCallback getWatchErrorCallback();
	void clearWatches();
	void waitWatchesThreadFinished();
};

}

#endif /* INCLUDE_XENSTORE_HPP_ */
