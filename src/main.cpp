#include "RingBuffer.h"
#include "AudioEngine.h"
#include "AnalyzerThread.h"
#include "GraphicsThread.h"
#include "TripleBuffer.h"

#include "constants.h"
#include <filesystem>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

std::atomic<bool> doneFlag(false);

namespace fs = std::filesystem;

int main()
{
	
	
	fs::path currentPath = fs::current_path();
	std::cout << "Current working directory" << currentPath << std::endl;
	std::string filePath = "demos/audio3.wav";

	TripleBuffer<std::vector<float>> tripleBuffer(Constants::BIN_COUNT);

	// SPSC lock-free ring buffer used by audio callback and analyzer
	RingBuffer ringBuffer = RingBuffer();

	AudioEngine audioObj = AudioEngine(ringBuffer, filePath);

	// launched as a functor in its overloaded operator()
	AnalyzerThread analyzerThread = AnalyzerThread(
		std::ref(ringBuffer),
		std::ref(tripleBuffer),
		std::ref(doneFlag)
	);

	analyzerThread.Launch();

	if (!audioObj.Init())
	{
		return EXIT_FAILURE;
	}
	audioObj.Start();

	GraphicsThread visualizer{ 1920,1080, tripleBuffer };
	visualizer.Initialize(); // initialize window and constants

	while (!WindowShouldClose())
	{
		BeginDrawing();
		visualizer.Update();
		EndDrawing();
	}

	doneFlag.store(true);

	return EXIT_SUCCESS;
}