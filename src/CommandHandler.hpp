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

#ifndef SRC_COMMANDHANDLER_HPP_
#define SRC_COMMANDHANDLER_HPP_

#include <vector>

#include <cstdint>

#include "AlsaPcm.hpp"

extern "C"
{
	#include "xenctrl.h"
	#include "sndif_linux.h"
}

class CommandHandler
{
public:
	CommandHandler(int domId, xc_gnttab* gnttab);
	~CommandHandler();

	uint8_t processCommand(const xensnd_req& req);

private:
	int mDomId;
	xc_gnttab* mGnttab;
	void* mBuffer;

	Alsa::AlsaPcm mAlsaPcm;

	typedef void(CommandHandler::*CommandFn)(const xensnd_req& req);

	std::vector<CommandFn> mCmdTable;

	void open(const xensnd_req& req);
	void close(const xensnd_req& req);
	void read(const xensnd_req& req);
	void write(const xensnd_req& req);

	void mapRefs(const grant_ref_t* refs);
	void unmapRefs();
};

#endif /* SRC_COMMANDHANDLER_HPP_ */
