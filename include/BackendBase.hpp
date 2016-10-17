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
#include <string>

extern "C"
{
	#include "xenstore.h"
	#include "xenctrl.h"
}

#include "FrontendHandlerBase.hpp"

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

	xs_handle* getXsHandle() const { return mXsHandle; }
	const std::string& getDeviceName() const { return mDeviceName; }
	const std::string& getXsDomPath() const { return mXsDomPath; }
	int getId() const { return mId; }
	int getDomId() const { return mDomId; }

protected:
	// TODO
	virtual int getNewFrontendId() {}

private:
	int			mId;
	int			mDomId;
	std::string mDeviceName;

	xs_handle*	mXsHandle;
	xc_gnttab*	mXcGnttab;

	std::string mXsDomPath;

	std::atomic_bool mTerminate;

	std::map<int, std::unique_ptr<FrontendHandlerBase>> mFrontendHandlers;

	void initXen();
	void releaseXen();
};

#endif /* INCLUDE_BACKENDBASE_HPP_ */
