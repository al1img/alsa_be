/*
 * BEBase.hpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#ifndef INCLUDE_BEBASE_HPP_
#define INCLUDE_BEBASE_HPP_

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

#include "FEHandlerBase.hpp"

class BEException : public std::exception
{
public:
	BEException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class BEBase
{
public:
	BEBase(int domId, const std::string& deviceName, int id = 0);
	virtual ~BEBase();

	void run();
	void stop();

	xs_handle* getXsh() const { return mXsh; }
	const std::string& getDeviceName() const { return mDeviceName; }
	const std::string& getXsDomPath() const { return mXsDomPath; }
	int getId() const { return mId; }
	int getDomId() const { return mDomId; }

protected:
	// TODO
	virtual int getNewFEId() {}

private:
	int			mId;
	int			mDomId;
	std::string mDeviceName;

	xs_handle*	mXsh;
	xc_gnttab*	mXcg;

	std::string mXsDomPath;

	std::atomic_bool mTerminate;

	std::map<int, std::unique_ptr<FEHandlerBase>> mFEHandlers;

	void releaseXen();
};

#endif /* INCLUDE_BEBASE_HPP_ */
