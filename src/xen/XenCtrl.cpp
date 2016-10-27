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

#include "XenCtrl.hpp"

#include <poll.h>

#include <glog/logging.h>

using std::string;
using std::to_string;
using std::vector;

namespace XenBackend {

XenInterface::XenInterface()
{
	try
	{
		init();
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenInterface::~XenInterface()
{
	release();
}

void XenInterface::getDomainsInfo(vector<xc_domaininfo_t>& infos)
{
	xc_domaininfo_t domainInfo[cDomInfoChunkSize];

	auto newDomains = 0, numDomains = 0;

	infos.clear();

	do
	{
		newDomains = xc_domain_getinfolist(mHandle, numDomains, cDomInfoChunkSize, domainInfo);

		if (newDomains < 0)
		{
			throw XenCtrlException("Can't get domain info");
		}

		numDomains += newDomains;

		for(auto i = 0; i < numDomains; i++)
		{
			infos.push_back(domainInfo[i]);
		}
	}
	while(newDomains == cDomInfoChunkSize);
}

void XenInterface::init()
{
	VLOG(1) << "Create xen interface";

	mHandle = xc_interface_open(0,0,0);

	if (!mHandle)
	{
		throw XenException("Can't open xc interface");
	}
}

void XenInterface::release()
{
	VLOG(1) << "Release xen interface";

	if (mHandle)
	{
		xc_interface_close(mHandle);
	}
}

XenEventChannel::XenEventChannel(int domId, int port)
{
	try
	{
		init(domId, port);
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenEventChannel::~XenEventChannel()
{
	release();
}

bool XenEventChannel::waitEvent()
{
	pollfd fds;

	fds.fd = xc_evtchn_fd(mHandle);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPoolEventTimeoutMs);

	if (ret > 0)
	{
		auto port = xc_evtchn_pending(mHandle);

		if (port < 0)
		{
			throw XenCtrlException("Can't get pending port");
		}

		if (xc_evtchn_unmask(mHandle, port) < 0)
		{
			throw XenCtrlException("Can't unmask event channel");
		}

		if (port != mPort)
		{
			throw XenCtrlException("Error port number: " + to_string(port) + ", expected: " + to_string(mPort));

			return false;
		}

		DVLOG(2) << "Event received, port: " << mPort;

		return true;
	}

	if (ret < 0)
	{
		throw XenCtrlException("Can't poll watches");
	}

	return false;
}

void XenEventChannel::notify()
{
	DVLOG(2) << "Notify event channel, port: " << mPort;

	if (xc_evtchn_notify(mHandle, mPort) < 0)
	{
		throw XenCtrlException("Can't notify event channel");
	}
}

void XenEventChannel::init(int domId, int port)
{
	mHandle = xc_evtchn_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenCtrlException("Can't open event channel");
	}

	mPort = xc_evtchn_bind_interdomain(mHandle, domId, port);

	if (mPort == -1)
	{
		throw XenCtrlException("Can't bind event channel");
	}

	VLOG(1) << "Create event channel, dom: " << domId << ", remote port: " << port << ", local port: " << mPort;
}

void XenEventChannel::release()
{
	if (mPort != -1)
	{
		xc_evtchn_unbind(mHandle, mPort);
	}

	if (mHandle)
	{
		xc_evtchn_close(mHandle);
	}

	VLOG(1) << "Delete event channel, local port: " << mPort;
}

XenGnttab::XenGnttab()
{
	mHandle = xc_gnttab_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenCtrlException("Can't open xc grant table");
	}
}

XenGnttab::~XenGnttab()
{
	if (mHandle)
	{
		xc_gnttab_close(mHandle);
	}
}

XenGnttabBuffer::XenGnttabBuffer(int domId, uint32_t ref, int prot) :
	mDomId(domId)
{
	init(&ref, 1, prot);
}

XenGnttabBuffer::XenGnttabBuffer(int domId, const uint32_t* refs, size_t count, int prot)
{
	init(refs, count, prot);
}

XenGnttabBuffer::~XenGnttabBuffer()
{
	release();
}

void XenGnttabBuffer::init(const uint32_t* refs, size_t count, int prot)
{
	static XenGnttab gnttab;

	mHandle = gnttab.getHandle();
	mBuffer = nullptr;
	mCount = count;

	VLOG(1) << "Create grant table buffer, dom: " << mDomId;

	mBuffer = xc_gnttab_map_domain_grant_refs(mHandle, count, mDomId, const_cast<uint32_t*>(refs), PROT_READ | PROT_WRITE);

}

void XenGnttabBuffer::release()
{
	VLOG(1) << "Delete grant table buffer, dom: " << mDomId;

	if (mBuffer)
	{
		xc_gnttab_munmap(mHandle, mBuffer, mCount);
	}
}

}
