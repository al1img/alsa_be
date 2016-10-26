/*
 * AlsaPcm.hpp
 *
 *  Created on: Oct 20, 2016
 *      Author: al1
 */

#ifndef SRC_ALSA_ALSAPCM_HPP_
#define SRC_ALSA_ALSAPCM_HPP_

#include <string>

#include <alsa/asoundlib.h>

namespace Alsa {

class AlsaPcmException : public std::exception
{
public:
	explicit AlsaPcmException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

struct AlsaPcmParams
{
	AlsaPcmParams(unsigned f, unsigned r, unsigned c) :
		format(f), rate(r), numChannels(c) {}

	unsigned			format;
	unsigned			rate;
	unsigned			numChannels;
};

class AlsaPcm
{
public:
	explicit AlsaPcm(const std::string& name = "default");
	~AlsaPcm();

	void open(const AlsaPcmParams& params, bool forCapture = false);
	void close();
	void read(void* buffer, ssize_t size);
	void write(const void* buffer, ssize_t size);
	void info();

private:
	snd_pcm_t *mHandle;
	std::string mName;

	void showCardInfo(int card);
	void showPcmDevicesInfo(snd_ctl_t* handle);
	void showPcmDeviceInfo(snd_ctl_t* handle, int dev, snd_pcm_stream_t stream);
	void showPcmSubdevicesInfo(snd_ctl_t* handle, snd_pcm_info_t *pcminfo, int subdeviceCount);
};

}

#endif /* SRC_ALSA_ALSAPCM_HPP_ */
