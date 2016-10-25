/*
 *  Xen Stat wrapper
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

#include "XenStat.hpp"

#include <glog/logging.h>

using std::vector;

namespace XenBackend {

XenStat::XenStat() :
	mHandle(nullptr)
{
	try
	{
		initHandle();
	}
	catch(const XenStatException& e)
	{
		releaseHandle();

		throw;
	}
}

vector<int> XenStat::getRunningDoms()
{
	xc_domaininfo_t domainInfo[cDomInfoChunkSize];

	vector<int> runningDomains;

	auto newDomains = 0, numDomains = 0;

	do
	{
		newDomains = xc_domain_getinfolist(mHandle, numDomains, cDomInfoChunkSize, domainInfo);

		if (newDomains < 0)
		{
			throw XenStatException("Can't get domain info");
		}

		numDomains += newDomains;

		for(auto i = 0; i < numDomains; i++)
		{
			if (domainInfo[i].flags & XEN_DOMINF_running)
			{
				runningDomains.push_back(domainInfo[i].domain);
			}
		}
	}
	while(newDomains == cDomInfoChunkSize);

	return runningDomains;
}

XenStat::~XenStat()
{
	releaseHandle();
}

void XenStat::initHandle()
{
	VLOG(2) << "Init xen stat";

	mHandle = xc_interface_open(0,0,0);

	if (!mHandle)
	{
		throw XenStatException("Can't open xc interface");
	}
}

void XenStat::releaseHandle()
{
	VLOG(2) << "Release xen stat";

	if (mHandle)
	{
		xc_interface_close(mHandle);
	}
}

}
