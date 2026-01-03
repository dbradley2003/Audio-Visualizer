#include <iostream>

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

AudioEngine::AudioEngine(RingBuffer& queue)
	:
	device(),
	decoder(),
	circularQueue(queue)
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

bool AudioEngine::LoadFile(const std::string& filePath)
{
	std::cout << "Loading " << filePath << std::endl;
	ma_decoder_config decoderConfig;

	decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
	ma_result result = ma_decoder_init_file(filePath.c_str(), &decoderConfig, &decoder);
	
	if (result != MA_SUCCESS) { return false; }

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = decoder.outputFormat;
	config.playback.channels = decoder.outputChannels;
	config.sampleRate = decoder.outputSampleRate;
	config.dataCallback = AudioEngine::ma_data_callback;
	config.pUserData = this;

	std::cout << "Sample Rate: " << config.sampleRate << std::endl;

	if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
	{
		std::cout << "error ocurred initializing device" << std::endl;
		ma_decoder_uninit(&decoder);
		return false;
	}

	return true;
}
