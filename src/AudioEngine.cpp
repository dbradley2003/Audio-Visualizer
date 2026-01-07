#include <iostream>
#include <string>

#include "AudioEngine.h"
#include "RingBuffer.h"


#pragma warning(disable : 4100)

void AudioEngine::ma_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	AudioEngine* pEngine = (AudioEngine*)pDevice->pUserData;

	float* pOutputF32 = (float*)pOutput;

	if (pEngine->decoder.pBackendVTable == NULL)
	{
		memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
		return;
	}

	ma_uint64 framesRead;
	ma_decoder_read_pcm_frames(&pEngine->decoder, pOutput, frameCount, &framesRead);

	if (framesRead < frameCount)
	{
		ma_decoder_seek_to_pcm_frame(&pEngine->decoder, 0);
		ma_uint64 framesRemaining = frameCount - framesRead;
		float* pNextOutputPart = pOutputF32 + (framesRead * pDevice->playback.channels);
		ma_uint64 extraFramesRead = 0;
		ma_decoder_read_pcm_frames(&pEngine->decoder, pNextOutputPart, framesRemaining, &extraFramesRead);
		framesRead += extraFramesRead;
	}

	for (ma_uint32 i{}; i < (ma_uint32)framesRead; ++i)
	{
		float left = pOutputF32[i * 2];
		float right = pOutputF32[i * 2 + 1];
		float monoSample = (left + right) * 0.5f;
		pEngine->circularQueue.PushBack(monoSample);
	}
}

AudioEngine::AudioEngine(RingBuffer& queue, std::string& filePath)
	:
	device(),
	decoder(),
	circularQueue(queue),
	filePath(std::move(filePath))
{
}

bool AudioEngine::isPlaying()
{
	return ma_device_get_state(&this->device) == ma_device_state_started;
}

AudioEngine::~AudioEngine()
{
	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
}

bool AudioEngine::Start()
{
	return ma_device_start(&device) == MA_SUCCESS;
}

bool AudioEngine::Init()
{
	ma_decoder_config decoderConfig;
	decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);

	if (ma_decoder_init_file(filePath.c_str(), &decoderConfig, &decoder) != MA_SUCCESS)
	{
		std::cerr << "Error ocurred initializing decoder" << std::endl;
		return false;
	}

	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate = decoder.outputSampleRate;
	deviceConfig.dataCallback = AudioEngine::ma_data_callback;
	deviceConfig.pUserData = this;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
	{
		std::cerr << "ERROR: Could not initialize miniaudio device" << std::endl;
		ma_decoder_uninit(&decoder);
		return false;
	}

	return true;
}
