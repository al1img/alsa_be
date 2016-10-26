/*
 * AlsaPcm.cpp
 *
 *  Created on: Oct 20, 2016
 *      Author: al1
 */

#include "AlsaPcm.hpp"

#include <exception>

#include <glog/logging.h>

using std::exception;
using std::string;
using std::to_string;

namespace Alsa {

AlsaPcm::AlsaPcm(const std::string& name) :
	mHandle(nullptr),
	mName(name)
{
	VLOG(1) << "Create pcm device: " << mName;
}

AlsaPcm::~AlsaPcm()
{
	VLOG(1) << "Delete pcm device: " << mName;

	close();
}

void AlsaPcm::open(const AlsaPcmParams& params, bool forCapture)
{
	snd_pcm_hw_params_t *hwParams = nullptr;

	try
	{
		VLOG(1) << "Open pcm device: " << mName;

		if (snd_pcm_open(&mHandle, mName.c_str(), forCapture ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK, 0) < 0)
		{
			throw AlsaPcmException("Can't open audio device " + mName);
		}

		if (snd_pcm_hw_params_malloc(&hwParams) < 0)
		{
			throw AlsaPcmException("Can't allocate hw params " + mName);
		}

		if (snd_pcm_hw_params_any(mHandle, hwParams) < 0)
		{
			throw AlsaPcmException("Can't allocate hw params " + mName);
		}

		if (snd_pcm_hw_params_set_access(mHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		{
			throw AlsaPcmException("Can't set access " + mName);
		}

		if (snd_pcm_hw_params_set_format(mHandle, hwParams, static_cast<snd_pcm_format_t>(params.format)) < 0)
		{
			throw AlsaPcmException("Can't set format " + mName);
		}

		unsigned int rate = params.rate;

		if (snd_pcm_hw_params_set_rate_near(mHandle, hwParams, &rate, 0) < 0)
		{
			throw AlsaPcmException("Can't set rate " + mName);
		}

		if (snd_pcm_hw_params_set_channels(mHandle, hwParams, params.numChannels) < 0)
		{
			throw AlsaPcmException("Can't set channels " + mName);
		}

		if (snd_pcm_hw_params(mHandle, hwParams) < 0)
		{
			throw AlsaPcmException("Can't set hwParams " + mName);
		}

	}
	catch(const exception& e)
	{
		if (hwParams)
		{
			snd_pcm_hw_params_free(hwParams);

			close();
		}

		throw;
	}
}

void AlsaPcm::close()
{
	VLOG(1) << "Close pcm device: " << mName;

	if (mHandle)
	{
		snd_pcm_close(mHandle);
	}
}

void AlsaPcm::read(void* buffer, ssize_t size)
{
	DVLOG(2) << "Read from pcm device: " << mName << ", size: " << size;

	auto numFrames = snd_pcm_bytes_to_frames(mHandle, size);

	if (snd_pcm_readi(mHandle, buffer, numFrames) != numFrames)
	{
		throw AlsaPcmException("Read from audio interface failed: " + mName);
	}
}

void AlsaPcm::write(const void* buffer, ssize_t size)
{
	DVLOG(2) << "Write to pcm device: " << mName << ", size: " << size;

	auto numFrames = snd_pcm_bytes_to_frames(mHandle, size);

	if (snd_pcm_writei(mHandle, buffer, numFrames) != numFrames)
	{
		throw AlsaPcmException("Write to audio interface failed: " + mName);
	}
}

void AlsaPcm::info()
{
	int card = -1;

	LOG(INFO) << "======================== Cards info ========================";

	do
	{
		if (snd_card_next(&card) < 0)
		{
			throw AlsaPcmException("Error getting next card");
		}

		if (card != -1)
		{
			showCardInfo(card);
		}
	}
	while(card != -1);

	LOG(INFO) << "============================================================";
}

void AlsaPcm::showCardInfo(int card)
{
	snd_ctl_t* handle = nullptr;
	snd_ctl_card_info_t *info = nullptr;

	try
	{
		snd_ctl_card_info_alloca(&info);

		string hwName("hw:" + to_string(card));

		if (snd_ctl_open(&handle, hwName.c_str(), 0) < 0)
		{
			throw AlsaPcmException("Error opening control");
		}

		if (snd_ctl_card_info(handle, info) < 0)
		{
			throw AlsaPcmException("Error getting card info");
		}

		LOG(INFO) << "Card: " << card;

		LOG(INFO) << "Id: " << snd_ctl_card_info_get_id(info)
				  << ", name: " << snd_ctl_card_info_get_name(info)
				  << ", long name: " << snd_ctl_card_info_get_longname(info);

		LOG(INFO) << "Driver: " << snd_ctl_card_info_get_driver(info)
				  << ", mixer name: " << snd_ctl_card_info_get_mixername(info);

		LOG(INFO) << "Components: " << snd_ctl_card_info_get_components(info);

		showPcmDevicesInfo(handle);

	}
	catch(const AlsaPcmException& e)
	{
		LOG(ERROR) << e.what();
	}

	if (handle)
	{
		snd_ctl_close(handle);
	}
}

void AlsaPcm::showPcmDevicesInfo(snd_ctl_t* handle)
{
	int dev = -1;

	do
	{
		if (snd_ctl_pcm_next_device(handle, &dev) < 0)
		{
			throw AlsaPcmException("Error getting next pcm device");
		}

		if (dev != -1)
		{
			LOG(INFO) << "\tDevice: " << dev;

			showPcmDeviceInfo(handle, dev, SND_PCM_STREAM_PLAYBACK);
			showPcmDeviceInfo(handle, dev, SND_PCM_STREAM_CAPTURE);
		}
	}
	while(dev != -1);
}

void AlsaPcm::showPcmDeviceInfo(snd_ctl_t* handle, int dev, snd_pcm_stream_t stream)
{
	snd_pcm_info_t *pcminfo = nullptr;

	snd_pcm_info_alloca(&pcminfo);

	snd_pcm_info_set_device(pcminfo, dev);
	snd_pcm_info_set_subdevice(pcminfo, 0);
	snd_pcm_info_set_stream(pcminfo, stream);

	auto status = snd_ctl_pcm_info(handle, pcminfo);

	if (status < 0)
	{
		if (status == -ENOENT)
		{
			return;
		}

		throw AlsaPcmException("Error getting pcm device info");
	}

	auto subdeviceCount = snd_pcm_info_get_subdevices_count(pcminfo);

	LOG(INFO) << "\tStream: " << snd_pcm_stream_name(stream);

	LOG(INFO) << "\t\tId: " << snd_pcm_info_get_id(pcminfo)
			  << ", name: " << snd_pcm_info_get_name(pcminfo)
			  << ", subdev count: " << subdeviceCount
			  << ", subdev avail: " << snd_pcm_info_get_subdevices_avail(pcminfo);

	showPcmSubdevicesInfo(handle, pcminfo, subdeviceCount);
}

void AlsaPcm::showPcmSubdevicesInfo(snd_ctl_t* handle, snd_pcm_info_t *pcminfo, int subdeviceCount)
{
	LOG(INFO) << "\t\t\tSubdev: 0, name: " << snd_pcm_info_get_subdevice_name(pcminfo);

	for (auto i = 1; i < subdeviceCount; i++)
	{
		snd_pcm_info_set_subdevice(pcminfo, i);

		if (snd_ctl_pcm_info(handle, pcminfo) < 0)
		{
			throw AlsaPcmException("Error getting pcm device info");
		}

		LOG(INFO) << "\t\t\tSubdev: " << i << ", name: " << snd_pcm_info_get_subdevice_name(pcminfo);
	}
}

}
