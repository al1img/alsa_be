/*
 *  Xen base ring buffer
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

#ifndef INCLUDE_RINGBUFFERBASE_HPP_
#define INCLUDE_RINGBUFFERBASE_HPP_

#include <functional>

extern "C" {
#include "xenctrl.h"
#include <xen/io/ring.h>
}

#include "XenException.hpp"
#include "XenGnttab.hpp"

namespace XenBackend {

class RingBufferException : public XenException
{
	using XenException::XenException;
};

class RingBufferItf
{
public:
	virtual ~RingBufferItf() {}
	virtual void onRequestReceived() = 0;
	virtual void setNotifyEventChannelCbk(std::function<void()> cbk) = 0;
};

template<typename Ring, typename SRing, typename Req, typename Rsp>
class RingBufferBase : public RingBufferItf
{
public:
	RingBufferBase(int domId, int ref, int pageSize) :
		mBuffer(domId, ref, PROT_READ | PROT_WRITE)
	{
		BACK_RING_INIT(&mRing, static_cast<SRing*>(mBuffer.get()), pageSize);
	}

protected:
	virtual void processRequest(const Req& req) = 0;

	void sendResponse(const Rsp& rsp)
	{
		bool notify = false;

		*RING_GET_RESPONSE(&mRing, mRing.rsp_prod_pvt) = rsp;

		mRing.rsp_prod_pvt++;

		RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&mRing, notify);

		if (notify)
		{
			mNotifyEventChannelCbk();
		}
	}

private:
	Ring mRing;
	XenGnttabBuffer mBuffer;
	std::function<void()> mNotifyEventChannelCbk;

	void setNotifyEventChannelCbk(std::function<void()> cbk)
	{
		mNotifyEventChannelCbk = cbk;
	}

	void onRequestReceived()
	{
		int numPendingRequests = 0;

		do {
			Req req;

			auto rc = mRing.req_cons;
			auto rp = mRing.sring->req_prod;

			xen_rmb();

			if (RING_REQUEST_PROD_OVERFLOW(&mRing, rp))
			{
				throw RingBufferException("Ring buffer producer overflow");
			}

			while (rc != rp) {

				if (RING_REQUEST_CONS_OVERFLOW(&mRing, rc))
				{
					throw RingBufferException("Ring buffer consumer overflow");
				}

				req = *RING_GET_REQUEST(&mRing, rc);

				mRing.req_cons = ++rc;

				xen_mb();

				processRequest(req);
			}

			RING_FINAL_CHECK_FOR_REQUESTS(&mRing, numPendingRequests);

		} while (numPendingRequests);
	}
};

}

#endif /* INCLUDE_RINGBUFFERBASE_HPP_ */
