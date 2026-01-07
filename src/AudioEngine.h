#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "miniaudio.h"
#include <string>

class RingBuffer;

class AudioEngine
{
public:
	AudioEngine(RingBuffer& queue, std::string& filePath);
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine(AudioEngine&&) = delete;
	AudioEngine& opertator(const AudioEngine&) = delete;
	AudioEngine& operator = (AudioEngine&&) = delete;
	~AudioEngine();

	bool Start();
	bool isPlaying();
	bool Init();
private:
	ma_device device;
	ma_decoder decoder;
	RingBuffer& circularQueue;
	std::string filePath;

	static void ma_data_callback(ma_device* pDevice, void* pOutput,
		const void* pInput, ma_uint32 frameCount);
};

#endif