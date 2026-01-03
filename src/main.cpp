#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

#include "RingBuffer.h"
#include "AudioEngine.h"
#include "AnalyzerThread.h"
#include "GraphicsThread.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

int main()
{
	std::mutex swapMtx;
	RingBuffer ringBuffer = RingBuffer();
	std::shared_ptr<std::vector<float>> shared;
	std::atomic<bool> dataReady(false);

	AnalyzerThread analyzerThread = AnalyzerThread(std::ref(ringBuffer),
		std::ref(shared), std::ref(dataReady), swapMtx);
	GraphicsThread graphicsThread = GraphicsThread(std::ref(shared),std::ref(dataReady),swapMtx);
	AudioEngine engine = AudioEngine(ringBuffer);

	std::thread t1(std::ref(analyzerThread));
	std::thread t2(std::ref(graphicsThread));
	if (engine.LoadFile("demos/audio4.wav"))
	{
		std::cout << "File Loaded" << std::endl;
		engine.Start();
	}

	t1.join();
	t2.join();
	return 0;
}