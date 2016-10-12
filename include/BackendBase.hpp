/*
 * BackendBase.hpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
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

	void releaseXen();
};

#endif /* INCLUDE_BACKENDBASE_HPP_ */
