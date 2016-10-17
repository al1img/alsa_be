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
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C"
{
	#include <xen/io/xenbus.h>
}

class BackendBase;

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
	FrontendHandlerBase(int domId, const BackendBase& backend);
	virtual ~FrontendHandlerBase();
	void start();
	void stop();

	virtual void onBind();

	void bindChannel(const std::string& name);

private:
	int mDomId;
	const BackendBase& mBackend;

	std::string mXsDomPath;
	std::string mXsBackendPath;
	std::string mXsFrontendPath;
	std::string mXsRemovePath;

	std::thread mThread;
	std::atomic_bool mTerminate;

	std::mutex mMutex;

	void run();

	void initXsPathes();
	void waitForBackendInitialized();
	void waitForFrontendInitialized();
	void waitForFrontendConnected();

	xenbus_state getState(const std::string& nodePath);
	xenbus_state waitForState(const std::string& nodePath, const std::vector<xenbus_state>& states);

	void setState(xenbus_state state);

	void setXsWatches();
	void clearXsWatches();
};


#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
