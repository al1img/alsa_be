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

#include "ChannelBase.hpp"

#include <glog/logging.h>

using std::string;

ChannelBase::ChannelBase(const std::string& name) :
	mName(name),
	mEventChannel(nullptr)
{
	try
	{
		LOG(INFO) << "Create channel: " << name;

		initXen();
	}
	catch(const ChannelException& e)
	{
		releaseXen();

		throw;
	}
}

void ChannelBase::initXen()
{
	mEventChannel = xc_evtchn_open(nullptr, 0);

	if (!mEventChannel)
	{
		throw ChannelException("Can't open event channel");
	}
}

void ChannelBase::releaseXen()
{
	if (mEventChannel)
	{
		xc_evtchn_close(mEventChannel);
	}
}

template<typename T>
Channel<T>::Channel(const string& name, int ringSize) : ChannelBase(name)
{

}
