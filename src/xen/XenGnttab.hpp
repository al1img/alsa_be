/*
 *  Xen gnttab wrapper
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

#ifndef SRC_XEN_XENGNTTAB_HPP_
#define SRC_XEN_XENGNTTAB_HPP_

#include <sys/mman.h>

extern "C" {
#include <xengnttab.h>
}

#include "XenException.hpp"

namespace XenBackend {

class XenGnttabException : public XenException
{
	using XenException::XenException;
};

class XenGnttab
{
private:
	friend class XenGnttabBuffer;

	XenGnttab();
	XenGnttab(const XenGnttab&) = delete;
	XenGnttab& operator=(XenGnttab const&) = delete;
	~XenGnttab();

	xengnttab_handle* getHandle() const { return mHandle; }

	xengnttab_handle* mHandle;
};

class XenGnttabBuffer
{
public:
	XenGnttabBuffer(int domId, uint32_t ref, int prot);
	XenGnttabBuffer(int domId, const uint32_t* refs, size_t count, int prot);
	XenGnttabBuffer(const XenGnttabBuffer&) = delete;
	XenGnttabBuffer& operator=(XenGnttabBuffer const&) = delete;
	~XenGnttabBuffer();

	void* get() const { return mBuffer; }

private:
	void* mBuffer;
	xengnttab_handle* mHandle;
	size_t mCount;
	int mDomId;

	void init(const uint32_t* refs, size_t count, int prot);
	void release();
};

}

#endif /* SRC_XEN_XENGNTTAB_HPP_ */
