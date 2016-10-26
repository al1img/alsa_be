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

using Alsa::AlsaPcmException;
using Alsa::AlsaPcmParams;

CommandHandler::CommandHandler(int domId, xc_gnttab* gnttab) :
	mDomId(domId),
	mGnttab(gnttab),
	mBuffer(nullptr),
	mCmdTable{&CommandHandler::open, &CommandHandler::close, &CommandHandler::read, &CommandHandler::write}
{
}

CommandHandler::~CommandHandler()
{

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

	return status;
}

void CommandHandler::open(const xensnd_req& req)
{
	DVLOG(2) << "Handle Open command";

	const xensnd_open_req& openReq = req.u.data.op.open;

	if (mBuffer)
	{
		mAlsaPcm.open(AlsaPcmParams(openReq.format, openReq.rate, openReq.channels));
	}

}

void CommandHandler::close(const xensnd_req& req)
{
	DVLOG(2) << "Handle Close command";
}

void CommandHandler::read(const xensnd_req& req)
{
	DVLOG(2) << "Handle Read command";
}

void CommandHandler::write(const xensnd_req& req)
{
	DVLOG(2) << "Handle Write command";
}

// TODO: create class to handle refs

void CommandHandler::mapRefs(const grant_ref_t* refs)
{
	mBuffer = xc_gnttab_map_domain_grant_refs(mGnttab, XENSND_MAX_PAGES_PER_REQUEST,
											  mDomId, const_cast<uint32_t*>(refs),
											  PROT_READ | PROT_WRITE);
}

void CommandHandler::unmapRefs()
{

}
