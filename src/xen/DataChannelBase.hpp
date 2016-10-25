/*
 *  Xen backend channel base class
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

#ifndef INCLUDE_DATACHANNELBASE_HPP_
#define INCLUDE_DATACHANNELBASE_HPP_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace XenBackend {

class EventChannel;
class RingBuffer;

class DataChannelException : public std::exception
{
public:
	DataChannelException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class FrontendHandlerBase;
class XenStore;

class DataChannelBase
{
public:
	DataChannelBase(const std::string& name, std::shared_ptr<EventChannel> eventChannel, std::shared_ptr<RingBuffer> ringBuffer);
	virtual ~DataChannelBase();

	void start();
	void stop();

	const std::string& getName() const { std::lock_guard<std::mutex> lock(mMutex); return mName; }

private:
	std::string mName;

	std::shared_ptr<EventChannel> mEventChannel;
	std::shared_ptr<RingBuffer> mRingBuffer;

	std::thread mThread;
	mutable std::mutex mMutex;
	std::atomic_bool mTerminate;

	void run();
};

}

#endif /* INCLUDE_DATACHANNELBASE_HPP_ */
