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

using std::string;
using std::to_string;
using std::vector;

namespace XenBackend {

XenStore::XenStore()
{
	try
	{
		initHandle();
	}
	catch(const XenStoreException& e)
	{
		releaseHandle();

		throw;
	}
}

XenStore::~XenStore()
{
	releaseHandle();
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

void XenStore::setWatch(const string& path)
{
	if (!xs_watch(mXsHandle, path.c_str(), ""))
	{
		throw XenStoreException("Can't set xs watch for " + path);
	}
}

void XenStore::clearWatch(const string& path)
{
	xs_unwatch(mXsHandle, path.c_str(), "");
}

bool XenStore::checkWatches()
{
	pollfd fds;

	fds.fd = xs_fileno(mXsHandle);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPollWatchesTimeoutMs);

	if (ret > 0)
	{
		char** result = nullptr;

		do
		{
			result = xs_check_watch(mXsHandle);

			if (result)
			{
				free(result);
			}
		}
		while(result);

		return true;
	}

	if (ret < 0)
	{
		throw XenStoreException("Can't poll watches");
	}

	return false;
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

void XenStore::initHandle()
{
	VLOG(2) << "Init xen store";

	mXsHandle = xs_open(0);

	if (!mXsHandle)
	{
		throw XenStoreException("Can't open xs daemon");
	}
}

void XenStore::releaseHandle()
{
	VLOG(2) << "Release xen store";

	if (mXsHandle)
	{
		xs_close(mXsHandle);
	}
}

}
