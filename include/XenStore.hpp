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

#include <exception>
#include <string>

extern "C"
{
	#include "xenstore.h"
}

class XenStoreException : public std::exception
{
public:
	XenStoreException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class XenStore
{
public:
	XenStore();
	~XenStore();

	std::string getDomainPath(int domId);
	int readInt(const std::string& path);
	void writeInt(const std::string& path, int value);

	void setWatch(const std::string& path);
	void clearWatch(const std::string& path);
	bool checkWatches();

private:
	xs_handle*	mXsHandle;

	void initHandle();
	void releaseHandle();
};

#endif /* INCLUDE_XENSTORE_HPP_ */
