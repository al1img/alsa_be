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

#include "BackendBase.hpp"
#include "CommandHandler.hpp"
#include "FrontendHandlerBase.hpp"
#include "RingBufferBase.hpp"

extern "C" {
#include "sndif_linux.h"
}

class AlsaFrontendHandler;

class StreamRingBuffer : public XenBackend::RingBufferBase<
											xen_sndif_back_ring,
											xen_sndif_sring,
											xensnd_req,
											xensnd_resp>
{
public:
	StreamRingBuffer(int id, Alsa::StreamType type, int domId, int ref);

private:
	int mId;
	CommandHandler mCommandHandler;

	void processRequest(const xensnd_req& req);
};

class AlsaFrontendHandler : public XenBackend::FrontendHandlerBase
{
	using XenBackend::FrontendHandlerBase::FrontendHandlerBase;

private:
	void onBind();

	void createStreamChannel(int id, Alsa::StreamType type, const std::string& streamPath);
	void processCard(const std::string& cardPath);
	void processDevice(const std::string& devPath);
	void processStream(const std::string& streamPath);
};

class AlsaBackend : public XenBackend::BackendBase
{
	using XenBackend::BackendBase::BackendBase;

private:

	// Uncomment for manual dom
	// bool getNewFrontend(int& domId, int& id);
	void onNewFrontend(int domId, int id);
};

#endif /* INCLUDE_ALSABACKEND_HPP_ */
