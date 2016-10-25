/*
 *  Xen event channel wrapper
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

#ifndef INCLUDE_EVENTCHANNEL_HPP_
#define INCLUDE_EVENTCHANNEL_HPP_

#include <exception>
#include <string>

extern "C"
{
	#include <xenctrl.h>
}

namespace XenBackend {

class FrontendHandlerBase;
class XenStore;

class EventChannelException : public std::exception
{
public:
	EventChannelException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class EventChannel
{
public:
	EventChannel(FrontendHandlerBase& frontendHandler, const std::string& portPath);
	~EventChannel();

	bool waitEvent();

	void notify();

private:
	const int cPoolEventTimeoutMs = 100;

	FrontendHandlerBase& mFrontendHandler;
	int mDomId;
	std::string mPortPath;
	xc_evtchn *mHandle;
	int mPort;
	XenStore& mXenStore;

	void initXen();
	void releaseXen();
};

}

#endif /* INCLUDE_EVENTCHANNEL_HPP_ */
