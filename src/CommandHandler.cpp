/*
 *  Xen alsa backend command handler
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

#include "CommandHandler.hpp"

#include <sys/mman.h>

#include <glog/logging.h>

using XenBackend::XenException;
using XenBackend::XenGnttabBuffer;

using Alsa::AlsaPcmException;
using Alsa::AlsaPcmParams;

CommandHandler::PcmFormat CommandHandler::sPcmFormat[] = {
	{ .sndif = XENSND_PCM_FORMAT_U8,                 .alsa = SND_PCM_FORMAT_U8 },
	{ .sndif = XENSND_PCM_FORMAT_S8,                 .alsa = SND_PCM_FORMAT_S8 },
	{ .sndif = XENSND_PCM_FORMAT_U16_LE,             .alsa = SND_PCM_FORMAT_U16_LE },
	{ .sndif = XENSND_PCM_FORMAT_U16_BE,             .alsa = SND_PCM_FORMAT_U16_BE },
	{ .sndif = XENSND_PCM_FORMAT_S16_LE,             .alsa = SND_PCM_FORMAT_S16_LE },
	{ .sndif = XENSND_PCM_FORMAT_S16_BE,             .alsa = SND_PCM_FORMAT_S16_BE },
	{ .sndif = XENSND_PCM_FORMAT_U24_LE,             .alsa = SND_PCM_FORMAT_U24_LE },
	{ .sndif = XENSND_PCM_FORMAT_U24_BE,             .alsa = SND_PCM_FORMAT_U24_BE },
	{ .sndif = XENSND_PCM_FORMAT_S24_LE,             .alsa = SND_PCM_FORMAT_S24_LE },
	{ .sndif = XENSND_PCM_FORMAT_S24_BE,             .alsa = SND_PCM_FORMAT_S24_BE },
	{ .sndif = XENSND_PCM_FORMAT_U32_LE,             .alsa = SND_PCM_FORMAT_U32_LE },
	{ .sndif = XENSND_PCM_FORMAT_U32_BE,             .alsa = SND_PCM_FORMAT_U32_BE },
	{ .sndif = XENSND_PCM_FORMAT_S32_LE,             .alsa = SND_PCM_FORMAT_S32_LE },
	{ .sndif = XENSND_PCM_FORMAT_S32_BE,             .alsa = SND_PCM_FORMAT_S32_BE },
	{ .sndif = XENSND_PCM_FORMAT_A_LAW,              .alsa = SND_PCM_FORMAT_A_LAW },
	{ .sndif = XENSND_PCM_FORMAT_MU_LAW,             .alsa = SND_PCM_FORMAT_MU_LAW },
	{ .sndif = XENSND_PCM_FORMAT_F32_LE,             .alsa = SND_PCM_FORMAT_FLOAT_LE },
	{ .sndif = XENSND_PCM_FORMAT_F32_BE,             .alsa = SND_PCM_FORMAT_FLOAT_BE },
	{ .sndif = XENSND_PCM_FORMAT_F64_LE,             .alsa = SND_PCM_FORMAT_FLOAT64_LE },
	{ .sndif = XENSND_PCM_FORMAT_F64_BE,             .alsa = SND_PCM_FORMAT_FLOAT64_BE },
	{ .sndif = XENSND_PCM_FORMAT_IEC958_SUBFRAME_LE, .alsa = SND_PCM_FORMAT_IEC958_SUBFRAME_LE },
	{ .sndif = XENSND_PCM_FORMAT_IEC958_SUBFRAME_BE, .alsa = SND_PCM_FORMAT_IEC958_SUBFRAME_BE },
	{ .sndif = XENSND_PCM_FORMAT_IMA_ADPCM,          .alsa = SND_PCM_FORMAT_IMA_ADPCM },
	{ .sndif = XENSND_PCM_FORMAT_MPEG,               .alsa = SND_PCM_FORMAT_MPEG },
	{ .sndif = XENSND_PCM_FORMAT_GSM,                .alsa = SND_PCM_FORMAT_GSM },
	{ .sndif = XENSND_PCM_FORMAT_SPECIAL,            .alsa = SND_PCM_FORMAT_SPECIAL },
};

CommandHandler::CommandHandler(Alsa::StreamType type, int domId) :
	mDomId(domId),
	mAlsaPcm(type),
	mCmdTable{&CommandHandler::open, &CommandHandler::close, &CommandHandler::read, &CommandHandler::write}
{
	VLOG(1) << "Create command handler, dom: " << mDomId;
}

CommandHandler::~CommandHandler()
{
	VLOG(1) << "Delete command handler, dom: " << mDomId;
}

uint8_t CommandHandler::processCommand(const xensnd_req& req)
{
	uint8_t status = XENSND_RSP_OKAY;

	try
	{
		if (req.u.data.operation < mCmdTable.size())
		{
			(this->*mCmdTable[req.u.data.operation])(req);
		}
		else
		{
			status = XENSND_RSP_ERROR;
		}
	}
	catch(const AlsaPcmException& e)
	{
		LOG(ERROR) << e.what();

		status = XENSND_RSP_ERROR;
	}

	DVLOG(2) << "Return status: [" << static_cast<int>(status) << "]";

	return status;
}

void CommandHandler::open(const xensnd_req& req)
{
	DVLOG(2) << "Handle command [OPEN]";

	const xensnd_open_req& openReq = req.u.data.op.open;

	mGnttab.reset(new XenGnttabBuffer(mDomId, openReq.grefs, XENSND_MAX_PAGES_PER_REQUEST, PROT_READ | PROT_WRITE));

	mAlsaPcm.open(AlsaPcmParams(convertPcmFormat(openReq.format), openReq.rate, openReq.channels));
}

void CommandHandler::close(const xensnd_req& req)
{
	DVLOG(2) << "Handle command [CLOSE]";

	mGnttab.reset();

	mAlsaPcm.close();
}

void CommandHandler::read(const xensnd_req& req)
{
	DVLOG(2) << "Handle command [READ]";

	const xensnd_read_req& readReq = req.u.data.op.read;

	mAlsaPcm.read(mGnttab->getBuffer(), readReq.len);
}

void CommandHandler::write(const xensnd_req& req)
{
	DVLOG(2) << "Handle command [WRITE]";

	const xensnd_write_req& writeReq = req.u.data.op.write;

	mAlsaPcm.write(mGnttab->getBuffer(), writeReq.len);
}

snd_pcm_format_t CommandHandler::convertPcmFormat(uint8_t format)
{
	for (auto value : sPcmFormat)
	{
		if (value.sndif == format)
		{
			return value.alsa;
		}
	}

	throw AlsaPcmException("Can't convert format");
}
