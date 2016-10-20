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

#include "DataChannelBase.hpp"

#include <exception>

#include <glog/logging.h>

#include "EventChannel.hpp"
#include "FrontendHandlerBase.hpp"
#include "RingBuffer.hpp"
#include "XenStore.hpp"

using std::exception;
using std::lock_guard;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;

namespace XenBackend {

DataChannelBase::DataChannelBase(const string& name, shared_ptr<EventChannel> eventChannel, shared_ptr<RingBuffer> ringBuffer) :
	mName(name),
	mEventChannel(eventChannel),
	mRingBuffer(ringBuffer)
{
	LOG(INFO) << "Create data channel: " << mName;

	mRingBuffer->setNotifyEventChannelCbk([this] () { mEventChannel->notify(); });
}

DataChannelBase::~DataChannelBase()
{
	stop();

	LOG(INFO) << "Delete data channel: " << mName;
}

void DataChannelBase::start()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Start data channel: " << mName;

	mThread = thread(&DataChannelBase::run, this);
}

void DataChannelBase::stop()
{
	lock_guard<mutex> lock(mMutex);

	LOG(INFO) << "Stop data channel: " << mName;

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

void DataChannelBase::run()
{
	try
	{
		while(!mTerminate)
		{
			if (mEventChannel->waitEvent())
			{
				mRingBuffer->onRequestReceived();
			}
		}
	}
	catch(const exception& e)
	{
		LOG(ERROR) << e.what();
	}
}

}
