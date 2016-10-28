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

	mAlsaPcm.open(AlsaPcmParams(openReq.format, openReq.rate, openReq.channels));
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
