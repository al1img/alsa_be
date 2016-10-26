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

#include <glog/logging.h>

using Alsa::AlsaPcmException;
using Alsa::AlsaPcmParams;

uint8_t CommandHandler::processCommand(const xensnd_req& req)
{
	uint8_t status = XENSND_RSP_OKAY;

	try
	{
		switch(req.u.data.operation)
		{
		case XENSND_OP_OPEN:
		{
			const xensnd_open_req& openReq = req.u.data.op.open;

			mAlsaPcm.open(AlsaPcmParams(openReq.format, openReq.rate, openReq.channels));

			break;
		}

		case XENSND_OP_CLOSE:

			mAlsaPcm.close();

			break;

		case XENSND_OP_READ:

			break;

		case XENSND_OP_WRITE:

			break;

		case XENSND_OP_SET_VOLUME:

			break;

		case XENSND_OP_GET_VOLUME:

			break;

		default:

			status = XENSND_RSP_ERROR;

			break;
		}
	}
	catch(const AlsaPcmException& e)
	{
		LOG(ERROR) << e.what();

		status = XENSND_RSP_ERROR;
	}

	return status;
}
