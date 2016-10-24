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
	mHandle(nullptr),
	mCurNode(nullptr)
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
	vector<int> result;

	for(auto i=0; i < xenstat_node_num_domains(mCurNode); i++)
	{
		auto domain = xenstat_node_domain_by_index(mCurNode,i);

		if (xenstat_domain_running(domain))
		{
			result.push_back(xenstat_domain_id(domain));
		}
	}

	return result;
}

XenStat::~XenStat()
{

}

void XenStat::initHandle()
{
	LOG(INFO) << "Init xen stat";

	mHandle = xenstat_init();

	if (mHandle == nullptr)
	{
		throw XenStatException("Failed to initialize xenstat library");
	}

	mCurNode = xenstat_get_node(mHandle, XENSTAT_ALL);

	if (mCurNode == nullptr)
	{
		throw XenStatException("Failed to retrieve statistics from libxenstat");
	}
}

void XenStat::releaseHandle()
{
	LOG(INFO) << "Release xen stat";

	if (mCurNode)
	{
		xenstat_free_node(mCurNode);
	}

	if (mHandle)
	{
		xenstat_uninit(mHandle);
	}
}

}
