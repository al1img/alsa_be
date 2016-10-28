/*
 *  Xen Ctrl wrapper
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


#ifndef SRC_XEN_XENCTRL_HPP_
#define SRC_XEN_XENCTRL_HPP_

#include <vector>

#include <sys/mman.h>

extern "C" {
#include <xenctrl.h>
}

#include "XenException.hpp"

namespace XenBackend {

class XenCtrlException : public XenException
{
	using XenException::XenException;
};

class XenInterface
{
public:
	XenInterface();
	~XenInterface();

	void getDomainsInfo(std::vector<xc_domaininfo_t>& infos);

private:
	const int cDomInfoChunkSize = 64;

	xc_interface* mHandle;

	void init();
	void release();
};

class XenEventChannel
{
public:
	XenEventChannel(int domId, int port);
	~XenEventChannel();

	bool waitEvent();
	void notify();

private:
	const int cPoolEventTimeoutMs = 100;

	xc_evtchn *mHandle;
	int mPort;

	void init(int domId, int port);
	void release();
};

class XenGnttab
{
public:
	XenGnttab();
	~XenGnttab();

	xc_gnttab* getHandle() const { return mHandle; }

private:
	xc_gnttab* mHandle;
};

class XenGnttabBuffer
{
public:
	XenGnttabBuffer(int domId, uint32_t ref, int prot);
	XenGnttabBuffer(int domId, const uint32_t* refs, size_t count, int prot);
	~XenGnttabBuffer();

	void* getBuffer() const { return mBuffer; }

private:
	void* mBuffer;
	xc_gnttab* mHandle;
	size_t mCount;
	int mDomId;

	void init(const uint32_t* refs, size_t count, int prot);
	void release();
};

}

#endif /* SRC_XEN_XENCTRL_HPP_ */
