/*
 *  Xen alsa backend
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

#ifndef INCLUDE_ALSABACKEND_HPP_
#define INCLUDE_ALSABACKEND_HPP_

extern "C"
{
	#include "xenctrl.h"
	#include "sndif_linux.h"
}

#include "BackendBase.hpp"
#include "EventChannel.hpp"
#include "CustomRingBuffer.hpp"
#include "FrontendHandlerBase.hpp"

class StreamRingBuffer : public XenBackend::CustomRingBuffer<
											xen_sndif_back_ring,
											xen_sndif_sring,
											xensnd_req,
											xensnd_resp>
{
public:
	enum class StreamType {PLAYBACK, CAPTURE};

	StreamRingBuffer(int id, StreamType type,
					 XenBackend::FrontendHandlerBase& frontendHandler,
					 const std::string& refPath);

private:
	int mId;
	StreamType mType;

	void processRequest(const xensnd_req& req);
};

class AlsaFrontendHandler : public XenBackend::FrontendHandlerBase
{
	using XenBackend::FrontendHandlerBase::FrontendHandlerBase;

private:
	void onBind();

	void createStreamChannel(int id, StreamRingBuffer::StreamType type, const std::string& streamPath);
	void processCard(const std::string& cardPath);
	void processDevice(const std::string& devPath);
	void processStream(const std::string& streamPath);
};

class AlsaBackend : public XenBackend::BackendBase
{
	using XenBackend::BackendBase::BackendBase;

private:

	int getNewFrontendId();

	void onNewFrontend(int domId);
};

#endif /* INCLUDE_ALSABACKEND_HPP_ */
