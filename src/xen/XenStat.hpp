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

#ifndef SRC_XEN_XENSTAT_HPP_
#define SRC_XEN_XENSTAT_HPP_

#include <vector>

#include "XenCtrl.hpp"
#include "XenException.hpp"

namespace XenBackend {

class XenStatException : public XenException
{
	using XenException::XenException;
};

class XenStat
{
public:
	XenStat();
	~XenStat();

	std::vector<uint32_t> getRunningDoms();

private:
	XenInterface mInterface;
};

}

#endif /* SRC_XEN_XENSTAT_HPP_ */