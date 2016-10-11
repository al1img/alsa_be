/*
 * BEAudio.hpp
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#ifndef INCLUDE_BEAUDIO_HPP_
#define INCLUDE_BEAUDIO_HPP_

#include "BEBase.hpp"

class BEAudio : public BEBase
{
	using BEBase::BEBase;

private:
	int getNewFEId() { return 1; }
};

#endif /* INCLUDE_BEAUDIO_HPP_ */
