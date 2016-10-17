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

#ifndef INCLUDE_CHANNELBASE_HPP_
#define INCLUDE_CHANNELBASE_HPP_

#include <string>

extern "C"
{
	#include <xenctrl.h>
}

class ChannelException : public std::exception
{
public:
	ChannelException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class ChannelBase
{
public:
	ChannelBase(const std::string& name);

private:
	std::string mName;

	xc_evtchn *mEventChannel;
	int mRefs;
	int mPort;

	void initXen();
	void releaseXen();
};

template<typename T>
class Channel : public ChannelBase
{
public:
	Channel(const std::string& name, int ringSize);
	virtual ~Channel();

private:
	T mRing;
};

#endif /* INCLUDE_CHANNELBASE_HPP_ */
