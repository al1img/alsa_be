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

#include <exception>

#include "DataChannel.hpp"
#include "FrontendHandlerBase.hpp"
#include "RingBufferBase.hpp"
#include "XenStore.hpp"

using std::exception;
using std::lock_guard;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;

namespace XenBackend {

DataChannel::DataChannel(const string& name, int domId, int port,
						 shared_ptr<RingBufferItf> ringBuffer) :
	mName(name),
	mEventChannel(domId, port),
	mRingBuffer(ringBuffer),
	mTerminate(false),
	mTerminated(false),
	mLog("DataChannel")
{
	LOG(mLog, DEBUG) << "Create data channel: " << mName;

	mRingBuffer->setNotifyEventChannelCbk([this] ()
										  { mEventChannel.notify(); });
}

DataChannel::~DataChannel()
{
	stop();

	LOG(mLog, DEBUG) << "Delete data channel: " << mName;
}

void DataChannel::start()
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Start data channel: " << mName;

	mThread = thread(&DataChannel::run, this);
}

void DataChannel::stop()
{
	LOG(mLog, DEBUG) << "Stop data channel: " << mName;

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

void DataChannel::run()
{
	try
	{
		while(!mTerminate)
		{
			if (mEventChannel.waitEvent())
			{
				mRingBuffer->onRequestReceived();
			}
		}
	}
	catch(const exception& e)
	{
		LOG(mLog, ERROR) << e.what();
	}

	mTerminated = true;
}

}
