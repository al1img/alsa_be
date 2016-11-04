/*
 * AlsaPcm.cpp
 *
 *  Created on: Oct 20, 2016
 *      Author: al1
 */

#include "AlsaPcm.hpp"

#include <exception>

using std::exception;
using std::string;
using std::to_string;

namespace Alsa {

AlsaPcm::AlsaPcm(StreamType type, const std::string& name) :
	mHandle(nullptr),
	mName(name),
	mType(type),
	mLog("AlsaPcm")
{
	LOG(mLog, DEBUG) << "Create pcm device: " << mName;
}

AlsaPcm::~AlsaPcm()
{
	LOG(mLog, DEBUG) << "Delete pcm device: " << mName;

	close();
}

void AlsaPcm::open(const AlsaPcmParams& params, bool forCapture)
{
	snd_pcm_hw_params_t *hwParams = nullptr;

	try
	{
		DLOG(mLog, DEBUG) << "Open pcm device: " << mName << ", format: " << params.format
				<< ", rate: " << params.rate << ", channels: " << params.numChannels;

		if (snd_pcm_open(&mHandle, mName.c_str(), mType == StreamType::PLAYBACK ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE, 0) < 0)
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

		if (snd_pcm_hw_params_set_format(mHandle, hwParams, params.format) < 0)
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

		if (snd_pcm_prepare(mHandle) < 0)
		{
			throw AlsaPcmException("Can't prepare audio interface for use");
		}
	}
	catch(const AlsaPcmException& e)
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
	DLOG(mLog, DEBUG) << "Close pcm device: " << mName;

	if (mHandle)
	{
		snd_pcm_drain(mHandle);
		snd_pcm_close(mHandle);
	}

	mHandle = nullptr;
}

void AlsaPcm::read(uint8_t* buffer, ssize_t size)
{
	DLOG(mLog, DEBUG) << "Read from pcm device: " << mName << ", size: " << size;

	auto numFrames = snd_pcm_bytes_to_frames(mHandle, size);

	while(numFrames > 0)
	{
		if (auto status = snd_pcm_readi(mHandle, buffer, numFrames))
		{
			if (status == -EPIPE)
			{
				LOG(mLog, WARNING) << "Device: " << mName << ", message: " << snd_strerror(status);

				snd_pcm_prepare(mHandle);
			}
			else if (status < 0)
			{
				throw AlsaPcmException("Read from audio interface failed: " + mName + ". Error: " + snd_strerror(status));
			}
			else
			{
				numFrames -= status;
				buffer = &buffer[snd_pcm_frames_to_bytes(mHandle, status)];
			}
		}
	}
}

void AlsaPcm::write(uint8_t* buffer, ssize_t size)
{
	DLOG(mLog, DEBUG) << "Write to pcm device: " << mName << ", size: " << size;

	auto numFrames = snd_pcm_bytes_to_frames(mHandle, size);

	while(numFrames > 0)
	{
		if (auto status = snd_pcm_writei(mHandle, buffer, numFrames))
		{
			if (status == -EPIPE)
			{
				LOG(mLog, WARNING) << "Device: " << mName << ", message: " << snd_strerror(status);

				snd_pcm_prepare(mHandle);
			}
			else if (status < 0)
			{
				throw AlsaPcmException("Write to audio interface failed: " + mName + ". Error: " + snd_strerror(status));
			}
			else
			{
				numFrames -= status;
				buffer = &buffer[snd_pcm_frames_to_bytes(mHandle, status)];
			}
		}
	}
}

void AlsaPcm::info()
{
	int card = -1;

	LOG(mLog, INFO) << "======================== Cards info ========================";

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

	LOG(mLog, INFO) << "============================================================";
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

		LOG(mLog, INFO) << "Card: " << card;

		LOG(mLog, INFO) << "Id: " << snd_ctl_card_info_get_id(info)
				  << ", name: " << snd_ctl_card_info_get_name(info)
				  << ", long name: " << snd_ctl_card_info_get_longname(info);

		LOG(mLog, INFO) << "Driver: " << snd_ctl_card_info_get_driver(info)
				  << ", mixer name: " << snd_ctl_card_info_get_mixername(info);

		LOG(mLog, INFO) << "Components: " << snd_ctl_card_info_get_components(info);

		showPcmDevicesInfo(handle);

	}
	catch(const AlsaPcmException& e)
	{
		LOG(mLog, ERROR) << e.what();
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
			LOG(mLog, INFO) << "\tDevice: " << dev;

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

	LOG(mLog, INFO) << "\tStream: " << snd_pcm_stream_name(stream);

	LOG(mLog, INFO) << "\t\tId: " << snd_pcm_info_get_id(pcminfo)
			  << ", name: " << snd_pcm_info_get_name(pcminfo)
			  << ", subdev count: " << subdeviceCount
			  << ", subdev avail: " << snd_pcm_info_get_subdevices_avail(pcminfo);

	showPcmSubdevicesInfo(handle, pcminfo, subdeviceCount);
}

void AlsaPcm::showPcmSubdevicesInfo(snd_ctl_t* handle, snd_pcm_info_t *pcminfo, int subdeviceCount)
{
	LOG(mLog, INFO) << "\t\t\tSubdev: 0, name: " << snd_pcm_info_get_subdevice_name(pcminfo);

	for (auto i = 1; i < subdeviceCount; i++)
	{
		snd_pcm_info_set_subdevice(pcminfo, i);

		if (snd_ctl_pcm_info(handle, pcminfo) < 0)
		{
			throw AlsaPcmException("Error getting pcm device info");
		}

		LOG(mLog, INFO) << "\t\t\tSubdev: " << i << ", name: " << snd_pcm_info_get_subdevice_name(pcminfo);
	}
}

}
