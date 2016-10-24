/*
 *  Xen ring buffer
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

#include "RingBuffer.hpp"

#include <glog/logging.h>

#include "FrontendHandlerBase.hpp"
#include "XenStore.hpp"

using std::string;

namespace XenBackend {

RingBuffer::RingBuffer(FrontendHandlerBase& frontendHandler, const std::string& refPath) :
	mFrontendHandler(frontendHandler),
	mDomId(frontendHandler.getDomId()),
	mRefPath(refPath),
	mXenStore(frontendHandler.getXenStore())
{
	try
	{
		LOG(INFO) << "Create ring buffer: " << mRefPath << ", dom: " << mDomId;

		initXen();
	}
	catch(const RingBufferException& e)
	{
		releaseXen();

		throw;
	}
}

RingBuffer::~RingBuffer()
{
	LOG(INFO) << "Delete ring buffer: " << mRefPath << ", dom: " << mDomId;

	releaseXen();
}

void RingBuffer::initXen()
{
	mRef = mXenStore.readInt(mRefPath);

	LOG(INFO) << "Read ref: " << mRefPath << ", dom: " << mDomId << ", ref: " << mRef;
}

void RingBuffer::releaseXen()
{

}

}

