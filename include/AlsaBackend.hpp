/*
 * AlsaBackend.hpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#ifndef INCLUDE_ALSABACKEND_HPP_
#define INCLUDE_ALSABACKEND_HPP_

#include "BackendBase.hpp"

class AlsaBackend : public BackendBase
{
	using BackendBase::BackendBase;

private:
	int getNewFrontendId() { return 1; }
};

#endif /* INCLUDE_ALSABACKEND_HPP_ */
