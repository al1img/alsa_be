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

#include <glog/logging.h>

extern "C"
{
	#include "xenctrl.h"
	#include "vsndif.h"
}

#include "BackendBase.hpp"
#include "EventChannel.hpp"
#include "CustomRingBuffer.hpp"
#include "FrontendHandlerBase.hpp"

class ControlChannel : public CustomRingBuffer<xen_vsndif_ctrl_back_ring,
											   xen_vsndif_ctrl_sring,
											   xen_vsndif_ctrl_request,
											   xen_vsndif_ctrl_response>
{
public:
	ControlChannel(FrontendHandlerBase& frontendHandler) :
		CustomRingBuffer<xen_vsndif_ctrl_back_ring,
						 xen_vsndif_ctrl_sring,
						 xen_vsndif_ctrl_request,
						 xen_vsndif_ctrl_response>(frontendHandler, "ring-ref", 4096)
	{

	}

private:
	void processRequest(const xen_vsndif_ctrl_request& req)
	{
		LOG(INFO) << "Request received: " << req.operation;

		xen_vsndif_ctrl_response rsp { .operation = req.operation, .status = 1 };

		sendResponse(rsp);
	}
};

class AlsaFrontendHandler : public FrontendHandlerBase
{
	using FrontendHandlerBase::FrontendHandlerBase;

private:

	void onBind()
	{
		std::shared_ptr<EventChannel> eventChannel(new EventChannel(*this, "evt-chnl"));
		std::shared_ptr<RingBuffer> ringBuffer(new ControlChannel(*this));

		addChannel(std::shared_ptr<DataChannelBase>(new DataChannelBase("ctrl", eventChannel, ringBuffer)));
	}
};

class AlsaBackend : public BackendBase
{
	using BackendBase::BackendBase;

private:

	int getNewFrontendId() { return 1; }

	void onNewFrontend(int domId)
	{
		addFrontendHandler(std::shared_ptr<FrontendHandlerBase>(new AlsaFrontendHandler(domId, *this, getXenStore())));
	}
};

#endif /* INCLUDE_ALSABACKEND_HPP_ */
