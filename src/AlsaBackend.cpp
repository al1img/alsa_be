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

#include "AlsaBackend.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include <getopt.h>
#include <signal.h>

#include "XenStore.hpp"

using std::cout;
using std::endl;
using std::exception;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using XenBackend::DataChannel;
using XenBackend::FrontendHandlerBase;
using XenBackend::Log;
using XenBackend::RingBufferItf;
using XenBackend::XenStore;

unique_ptr<AlsaBackend> alsaBackend;

StreamRingBuffer::StreamRingBuffer(int id, Alsa::StreamType type, int domId, int ref) :
	RingBufferBase<xen_sndif_back_ring, xen_sndif_sring, xensnd_req, xensnd_resp>(domId, ref),
	mId(id),
	mCommandHandler(type, domId),
	mLog("StreamRing(" + to_string(id) + ")")
{
	LOG(mLog, DEBUG) << "Create stream ring buffer: id = " << id << ", type:" << static_cast<int>(type);
}

void StreamRingBuffer::processRequest(const xensnd_req& req)
{
	DLOG(mLog, DEBUG) << "Request received, id: " << mId << ", cmd:" << static_cast<int>(req.u.data.operation);

	xensnd_resp rsp {};

	rsp.u.data.id = req.u.data.id;
	rsp.u.data.stream_idx = req.u.data.stream_idx;
	rsp.u.data.operation = req.u.data.operation;
	rsp.u.data.status = mCommandHandler.processCommand(req);

	sendResponse(rsp);
}

void AlsaFrontendHandler::onBind()
{
	string cardBasePath = getXsFrontendPath() + "/" + XENSND_PATH_CARD;

	const vector<string> cards = getXenStore().readDirectory(cardBasePath);

	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	if (cards.size() == 0)
	{
		LOG(mLog, WARNING) << "No sound cards found : " << getDomId();
	}

	for(auto cardId : cards)
	{
		LOG(mLog, DEBUG) << "Found card: " << cardId;

		processCard(cardBasePath + "/" + cardId);
	}
}

void AlsaFrontendHandler::processCard(const std::string& cardPath)
{
	string devBasePath = cardPath + "/" + XENSND_PATH_DEVICE;

	const vector<string> devs = getXenStore().readDirectory(devBasePath);

	for(auto devId : devs)
	{
		LOG(mLog, DEBUG) << "Found device: " << devId;

		processDevice(devBasePath + "/" + devId);
	}
}

void AlsaFrontendHandler::processDevice(const std::string& devPath)
{
	string streamBasePath = devPath + "/" + XENSND_PATH_STREAM;

	const vector<string> streams = getXenStore().readDirectory(streamBasePath);

	for(auto streamId : streams)
	{
		LOG(mLog, DEBUG) << "Found stream: " << streamId;

		processStream(streamBasePath + "/" + streamId);
	}
}

void AlsaFrontendHandler::processStream(const std::string& streamPath)
{
	int id = getXenStore().readInt(streamPath + "/" + XENSND_FIELD_STREAM_INDEX);
	Alsa::StreamType streamType = Alsa::StreamType::PLAYBACK;

	if (getXenStore().readString(streamPath + "/" + XENSND_FIELD_TYPE) == XENSND_STREAM_TYPE_CAPTURE)
	{
		streamType = Alsa::StreamType::CAPTURE;
	}

	createStreamChannel(id, streamType, streamPath);
}

void AlsaFrontendHandler::createStreamChannel(int id, Alsa::StreamType type, const string& streamPath)
{
	auto port = getXenStore().readInt(streamPath + "/" + XENSND_FIELD_EVT_CHNL);

	LOG(mLog, DEBUG) << "Read event channel port: " << port << ", dom: " << getDomId();

	uint32_t ref = getXenStore().readInt(streamPath + "/" + XENSND_FIELD_RING_REF);

	LOG(mLog, DEBUG) << "Read ring buffer ref: " << ref << ", dom: " << getDomId();


	shared_ptr<RingBufferItf> ringBuffer(new StreamRingBuffer(id, type, getDomId(), ref));

	addChannel(shared_ptr<DataChannel>(new DataChannel("stream " + to_string(id), getDomId(), port, ringBuffer)));
}

// Uncomment for manual dom

/*
bool AlsaBackend::getNewFrontend(int& domId, int& id)
{
	domId = 1;
	id = 0;

	return true;
}
*/

void AlsaBackend::onNewFrontend(int domId, int id)
{
	addFrontendHandler(shared_ptr<FrontendHandlerBase>(new AlsaFrontendHandler(domId, *this, id)));
}

void terminate(int sig, siginfo_t *info, void *ptr)
{
	alsaBackend->stop();
}

void segHandler(int sig)
{
	LOG("Main", ERROR) << "Unknown error!";
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

bool commandLineOptions(int argc, char *argv[])
{

	int opt = -1;

	while((opt = getopt(argc, argv, "v:fh?")) != -1)
	{
		switch(opt)
		{
		case 'v':
			if (!Log::setLogLevel(string(optarg)))
			{
				return false;
			}

			break;

		case 'f':
			Log::setShowFileAndLine(true);
			break;

		default:
			return false;
		}
	}

	return true;
}

int main(int argc, char *argv[])
{
	try
	{
		registerTerminate();

		if (commandLineOptions(argc, argv))
		{
			alsaBackend.reset(new AlsaBackend(0, XENSND_DRIVER_NAME));

			alsaBackend->run();
		}
		else
		{
			cout << "Usage: " << argv[0] << " [-v <level>]" << endl;
			cout << "\t-v -- verbose level (disable, error, warning, info, debug)" << endl;
		}
	}
	catch(const exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}
	catch(...)
	{
		LOG("Main", ERROR) << "Unknown error";
	}

	return 0;
}
