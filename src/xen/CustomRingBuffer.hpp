/*
 *  Xen custom ring buffer
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

#ifndef INCLUDE_CUSTOMRINGBUFFER_HPP_
#define INCLUDE_CUSTOMRINGBUFFER_HPP_

#include "RingBuffer.hpp"

#include <cstring>

#include <sys/mman.h>

extern "C"
{
	#include <xenctrl.h>
}

namespace XenBackend {

template<typename Ring, typename SRing, typename Req, typename Rsp>
class CustomRingBuffer : public RingBuffer
{
public:
	CustomRingBuffer(FrontendHandlerBase& frontendHandler, const std::string& ringRefPath, int pageSize) :
		RingBuffer(frontendHandler, ringRefPath),
		mGnttab(mFrontendHandler.getXcGnttab()),
		mPageSize(pageSize)
	{
		mRing.sring = static_cast<SRing*>(xc_gnttab_map_grant_ref(mGnttab, mDomId, mRef, PROT_READ | PROT_WRITE));

		if (!mRing.sring)
		{
			throw RingBufferException("Can't map grant reference");
		}

		BACK_RING_INIT(&mRing, mRing.sring, mPageSize);
	}

	~CustomRingBuffer()
	{
		if (mRing.sring)
		{
			xc_gnttab_munmap(mGnttab, mRing.sring, 1);
		}
	}

protected:
	virtual void processRequest(const Req& req) = 0;

	void sendResponse(const Rsp& rsp)
	{
		int notify = 0;

		std::memcpy(RING_GET_RESPONSE(&mRing, mRing.rsp_prod_pvt), &rsp, sizeof(rsp));

		mRing.rsp_prod_pvt++;

		RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&mRing, notify);

		if (notify)
		{
			mNotifyEventChannelCbk();
		}
	}

private:
	xc_gnttab* mGnttab;
	int mPageSize;

	Ring mRing;

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
				throw RingBufferException("Frontend provided bogus ring request");
			}

			while (rc != rp) {

				if (RING_REQUEST_CONS_OVERFLOW(&mRing, rc))
				{
					LOG(ERROR) << "Ring buffer overflow";

					break;
				}

				std::memcpy(&req, RING_GET_REQUEST(&mRing, rc), sizeof(req));

				mRing.req_cons = ++rc;

				xen_mb();

				processRequest(req);
			}

			RING_FINAL_CHECK_FOR_REQUESTS(&mRing, numPendingRequests);

		} while (numPendingRequests);
	}
};

}

#endif /* INCLUDE_CUSTOMRINGBUFFER_HPP_ */
