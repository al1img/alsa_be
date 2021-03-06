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

using std::vector;

namespace XenBackend {

XenStat::XenStat() :
	mLog("XenStat")
{
	LOG(mLog, DEBUG) << "Init xen stat";
}

XenStat::~XenStat()
{
	LOG(mLog, DEBUG) << "Release xen stat";
}

vector<uint32_t> XenStat::getRunningDoms()
{
	vector<uint32_t> runningDomains;

	vector<xc_domaininfo_t> domInfos;

	mInterface.getDomainsInfo(domInfos);

	for(auto info : domInfos)
	{
		if (info.flags & XEN_DOMINF_running)
		{
			runningDomains.push_back(info.domain);
		}
	}

	return runningDomains;
}

vector<uint32_t> XenStat::getExistingDoms()
{
	vector<uint32_t> existingDomains;

	vector<xc_domaininfo_t> domInfos;

	mInterface.getDomainsInfo(domInfos);

	for(auto info : domInfos)
	{
		existingDomains.push_back(info.domain);
	}

	return existingDomains;
}

}
