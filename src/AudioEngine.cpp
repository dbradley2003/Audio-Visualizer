#include "AudioEngine.h"
#include <iostream>
#include <string>

void AudioEngine::ma_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	auto* pEngine = static_cast<AudioEngine *>(pDevice->pUserData);

	auto* pOutputF32 = static_cast<float*>(pOutput);

	if (pEngine->decoder.pBackendVTable == NULL) {
		memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
		return;
	}

	ma_uint64 framesRead;
	ma_decoder_read_pcm_frames(&pEngine->decoder, pOutput, frameCount, &framesRead);

	if (framesRead < frameCount) {
		ma_decoder_seek_to_pcm_frame(&pEngine->decoder, 0);
		const ma_uint64 framesRemaining = frameCount - framesRead;


		//float* pNextOutputPart = pOutputF32 + (framesRead * pDevice->playback.channels);
		float* pNextOutputPart = pOutputF32 + framesRead;

		ma_uint64 extraFramesRead = 0;
		ma_decoder_read_pcm_frames(&pEngine->decoder, pNextOutputPart, framesRemaining, &extraFramesRead);
		framesRead += extraFramesRead;
	}

	for (ma_uint32 i{}; i < static_cast<ma_uint32>(framesRead); ++i) {
		//const float left = pOutputF32[i * 2];
		//const float right = pOutputF32[i * 2 + 1];
		//const float monoSample = (left + right) * 0.5f;
		//pEngine->circularQueue.PushBack(monoSample);
		const float sample = pOutputF32[i];
		pEngine->circularQueue.PushBack(sample);
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
	decoderConfig = ma_decoder_config_init(ma_format_f32, 1, 0);

	if (ma_decoder_init_file(filePath.c_str(), &decoderConfig, &decoder) != MA_SUCCESS)
	{
		std::cerr << "Error occurred initializing decoder" << std::endl;
		return false;
	}

	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate = decoder.outputSampleRate;
	deviceConfig.dataCallback = ma_data_callback;
	deviceConfig.pUserData = this;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
	{
		std::cerr << "ERROR: Could not initialize miniaudio device" << std::endl;
		ma_decoder_uninit(&decoder);
		return false;
	}

	return true;
}
