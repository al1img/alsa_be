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

using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;
using std::vector;

namespace XenBackend {

XenStore::XenStore()
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

	waitHandlerFinished();

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

void XenStore::removePath(const std::string& path)
{
	if (!xs_rm(mXsHandle, XBT_NULL, path.c_str()))
	{
		throw XenStoreException("Can't remove path " + path);
	}
}

void XenStore::setWatch(const std::string& path, std::function<void(const std::string& path)> callback)
{
	lock_guard<mutex> itfLock(mItfMutex);

	if (!xs_watch(mXsHandle, path.c_str(), ""))
	{
		throw XenStoreException("Can't set xs watch for " + path);
	}

	lock_guard<mutex> lock(mMutex);

	if (mWatches.size() == 0)
	{
		mThread = thread(&XenStore::handleWatches, this);
	}

	mWatches[path] = callback;
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
		waitHandlerFinished();
	}
}

const vector<string> XenStore::readDirectory(const string& path)
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

bool XenStore::checkIfExist(const std::string& path)
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

bool XenStore::checkWatches(string& retPath, string& retToken)
{
	int ret = 1;

	do
	{
		auto result = xs_check_watch(mXsHandle);

		if (result)
		{
			retPath = result[XS_WATCH_PATH];
			retToken = result[XS_WATCH_TOKEN];

			free(result);

			return true;
		}

		pollfd fds = { .fd = xs_fileno(mXsHandle), .events = POLLIN};

		ret = poll(&fds, 1, cPollWatchesTimeoutMs);

		if (ret < 0)
		{
			LOG(ERROR) << "Can't poll watches";
		}
	}
	while(ret > 0);

	return false;
}

void XenStore::handleWatches()
{
	bool terminate = false;

	while(!terminate)
	{
		string path, token;

		if (checkWatches(path, token))
		{
			if (mWatches.find(path) != mWatches.end())
			{
				VLOG(1) << "Watch triggered: " << path << ", " << token;

				mWatches[path](path);
			}
		}
		else
		{
			lock_guard<mutex> lock(mMutex);

			if (mWatches.empty())
			{
				terminate = true;
			}
		}
	}
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

void XenStore::waitHandlerFinished()
{
	VLOG(1) << "Wait for watch handler finished";

	if (mThread.joinable())
	{
		mThread.join();
	}
}

}
