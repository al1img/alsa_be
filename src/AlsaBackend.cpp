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

#include <signal.h>

#include <glog/logging.h>

#include "AlsaBackend.hpp"

using std::exception;
using std::runtime_error;
using std::shared_ptr;
using std::unique_ptr;

using XenBackend::DataChannelBase;
using XenBackend::EventChannel;
using XenBackend::FrontendHandlerBase;
using XenBackend::RingBuffer;

unique_ptr<AlsaBackend> alsaBackend;

ControlChannel::ControlChannel(FrontendHandlerBase& frontendHandler) :
	CustomRingBuffer<xen_vsndif_ctrl_back_ring,
					 xen_vsndif_ctrl_sring,
					 xen_vsndif_ctrl_request,
					 xen_vsndif_ctrl_response>(frontendHandler, "ring-ref", 4096)
{

}

void ControlChannel::processRequest(const xen_vsndif_ctrl_request& req)
{
	LOG(INFO) << "Request received: " << req.operation;

	xen_vsndif_ctrl_response rsp { .operation = req.operation, .status = 1 };

	sendResponse(rsp);
}

void AlsaFrontendHandler::onBind()
{
	shared_ptr<EventChannel> eventChannel(new EventChannel(*this, "evt-chnl"));
	shared_ptr<RingBuffer> ringBuffer(new ControlChannel(*this));

	addChannel(shared_ptr<DataChannelBase>(new DataChannelBase("ctrl", eventChannel, ringBuffer)));
}

int AlsaBackend::getNewFrontendId()
{
	return 1;
}

void AlsaBackend::onNewFrontend(int domId)
{
	addFrontendHandler(shared_ptr<FrontendHandlerBase>(new AlsaFrontendHandler(domId, *this, getXenStore())));
}

void terminate(int sig, siginfo_t *info, void *ptr)
{
	alsaBackend->stop();
}

void segHandler(int sig)
{
	LOG(FATAL) << "Unknown error!";
}

void registerTerminate()
{
	struct sigaction action;

	action.sa_sigaction = terminate;
	sigfillset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &action, NULL) < 0)
	{
		throw runtime_error(strerror(errno));
	}

	if (sigaction(SIGTERM, &action, NULL) < 0)
	{
		throw runtime_error(strerror(errno));
	}

	signal(SIGSEGV, segHandler);
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);

	try
	{
		registerTerminate();

		alsaBackend.reset(new AlsaBackend(0, "audio"));

		alsaBackend->run();
	}
	catch(const exception& e)
	{
		LOG(ERROR) << e.what();
	}
	catch(...)
	{
		LOG(ERROR) << "Unknown error";
	}

	return 0;
}