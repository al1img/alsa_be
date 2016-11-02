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
#include "XenStore.hpp"

#include <poll.h>

#include <glog/logging.h>

using std::exception;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;
using std::vector;

namespace XenBackend {

XenStore::XenStore() :
	mCheckWatchResult(false)
{
	try
	{
		init();
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenStore::~XenStore()
{
	clearWatches();

	waitWatchesThreadFinished();

	release();
}

string XenStore::getDomainPath(int domId)
{
	auto domPath = xs_get_domain_path(mXsHandle, domId);

	if (!domPath)
	{
		throw XenStoreException("Can't get domain path");
	}

	string result(domPath);

	free(domPath);

	return result;
}

int XenStore::readInt(const string& path)
{
	unsigned length;
	auto pData = static_cast<char*>(xs_read(mXsHandle, XBT_NULL, path.c_str(), &length));

	if (!pData)
	{
		throw XenStoreException("Can't read int from: " + path);
	}

	string result(pData);

	free(pData);

	return stoi(result);
}

string XenStore::readString(const string& path)
{
	unsigned length;
	auto pData = static_cast<char*>(xs_read(mXsHandle, XBT_NULL, path.c_str(), &length));

	if (!pData)
	{
		throw XenStoreException("Can't read string from: " + path);
	}

	string result(pData);

	free(pData);

	return result;
}

void XenStore::writeInt(const string& path, int value)
{
	auto strValue = to_string(value);

	if (!xs_write(mXsHandle, XBT_NULL, path.c_str(), strValue.c_str(), strValue.length()))
	{
		throw XenStoreException("Can't write value to " + path);
	}
}

void XenStore::removePath(const string& path)
{
	if (!xs_rm(mXsHandle, XBT_NULL, path.c_str()))
	{
		throw XenStoreException("Can't remove path " + path);
	}
}

vector<string> XenStore::readDirectory(const string& path)
{
	unsigned int num;
	auto items = xs_directory(mXsHandle, XBT_NULL, path.c_str(), &num);

	if (items && num)
	{
		vector<string> result;

		result.reserve(num);

		for(auto i = 0; i < num; i++)
		{
			result.push_back(items[i]);
		}

		free(items);

		return result;
	}

	return vector<string>();
}

bool XenStore::checkIfExist(const string& path)
{
	unsigned length;
	auto pData = xs_read(mXsHandle, XBT_NULL, path.c_str(), &length);

	if (!pData)
	{
		return false;
	}

	free(pData);

	return true;
}

void XenStore::setWatch(const string& path, WatchCallback callback, bool initNotify)
{
	lock_guard<mutex> itfLock(mItfMutex);

	if (!xs_watch(mXsHandle, path.c_str(), ""))
	{
		throw XenStoreException("Can't set xs watch for " + path);
	}

	lock_guard<mutex> lock(mMutex);

	if (initNotify)
	{
		mInitNotifyWatches.push_back(path);
	}

	if (mWatches.size() == 0)
	{
		mWatches[path] = callback;

		mThread = thread(&XenStore::watchesThread, this);
	}
}

void XenStore::clearWatch(const string& path)
{
	lock_guard<mutex> itfLock(mItfMutex);

	xs_unwatch(mXsHandle, path.c_str(), "");

	{
		lock_guard<mutex> lock(mMutex);

		mWatches.erase(path);
	}

	if (mWatches.empty())
	{
		waitWatchesThreadFinished();
	}
}

void XenStore::setWatchErrorCallback(WatchErrorCallback errorCallback)
{
	lock_guard<mutex> lock(mMutex);

	mErrorCallback = errorCallback;
}

void XenStore::init()
{
	VLOG(1) << "Init xen store";

	mXsHandle = xs_open(0);

	if (!mXsHandle)
	{
		throw XenStoreException("Can't open xs daemon");
	}
}

void XenStore::release()
{
	VLOG(1) << "Release xen store";

	if (mXsHandle)
	{
		xs_close(mXsHandle);
	}
}

string XenStore::checkWatches()
{
	string path;

	if (!mCheckWatchResult)
	{
		mCheckWatchResult = pollXsWatchFd();
	}

	if (mCheckWatchResult)
	{
		path = checkXsWatch();

		mCheckWatchResult = !path.empty();
	}

	return path;
}

string XenStore::checkXsWatch()
{
	string path;

	auto result = xs_check_watch(mXsHandle);

	if (result)
	{
		path = result[XS_WATCH_PATH];

		free(result);
	}

	return path;
}

bool XenStore::pollXsWatchFd()
{
	pollfd fds = { .fd = xs_fileno(mXsHandle), .events = POLLIN};

	auto ret = poll(&fds, 1, cPollWatchesTimeoutMs);

	if (ret < 0)
	{
		throw XenStoreException("Can't poll watches");
	}

	if (ret > 0)
	{
		return true;
	}

	return false;
}

void XenStore::watchesThread()
{
	try
	{
		while(!isWatchesEmpty())
		{
			string path = getInitNotifyPath();

			if (path.empty())
			{
				path = checkWatches();
			}

			if (!path.empty())
			{
				auto callback = getWatchCallback(path);

				if (callback)
				{
					callback(path);
				}
			}
		}
	}
	catch(const exception& e)
	{
		auto errorCbk = getWatchErrorCallback();

		if (errorCbk)
		{
			errorCbk(e);
		}
		else
		{
			LOG(ERROR) << e.what();
		}
	}
}

string XenStore::getInitNotifyPath()
{
	string path;

	if (mInitNotifyWatches.size())
	{
		path = mInitNotifyWatches.front();
		mInitNotifyWatches.pop_front();
	}

	return path;
}

XenStore::WatchCallback XenStore::getWatchCallback(string& path)
{
	lock_guard<mutex> lock(mMutex);

	WatchCallback callback = nullptr;

	auto result = mWatches.find(path);
	if (result != mWatches.end())
	{
		VLOG(1) << "Watch triggered: " << path;

		callback = result->second;
	}

	return callback;
}

XenStore::WatchErrorCallback XenStore::getWatchErrorCallback()
{
	lock_guard<mutex> lock(mMutex);

	return mErrorCallback;
}

bool XenStore::isWatchesEmpty()
{
	lock_guard<mutex> lock(mMutex);

	if (mWatches.empty())
	{
		return true;
	}

	return false;
}

void XenStore::clearWatches()
{
	VLOG(1) << "Clear watches";

	for (auto watch : mWatches)
	{
		xs_unwatch(mXsHandle, watch.first.c_str(), "");
	}

	lock_guard<mutex> lock(mMutex);

	mWatches.clear();
}

void XenStore::waitWatchesThreadFinished()
{
	VLOG(1) << "Wait for watch handler finished";

	if (mThread.joinable())
	{
		mThread.join();
	}
}

}
