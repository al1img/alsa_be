/*
 * FrontendHandlerBase.h
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
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
	void waitForBackendIntialized();
	void waitForFrontendInitialized();
	void waitForFrontendConnected();

	xenbus_state getState(const std::string& nodePath);
	xenbus_state waitForState(const std::string& nodePath, const std::vector<xenbus_state>& states);

	void setState(xenbus_state state);

	void setXsWatches();
	void clearXsWatches();
};


#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
