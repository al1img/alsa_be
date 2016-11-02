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

#include <glog/logging.h>

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

}
