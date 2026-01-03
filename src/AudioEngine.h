#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "miniaudio.h"
#include <string>

class RingBuffer;

class AudioEngine
{
public:
	AudioEngine(RingBuffer& queue);
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine(AudioEngine&&) = delete;
	AudioEngine& opertator(const AudioEngine&) = delete;
	AudioEngine& operator = (AudioEngine&&) = delete;
	~AudioEngine();

	bool LoadFile(const std::string& filePath);
	bool Start();
	bool isPlaying();
private:
	ma_device device;
	ma_decoder decoder;
	RingBuffer& circularQueue;

	static void ma_data_callback(ma_device* pDevice, void* pOutput,
		const void* pInput, ma_uint32 frameCount);
};

#endif