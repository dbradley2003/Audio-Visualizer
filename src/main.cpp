#include <iostream>

#include "RingBuffer.h"
#include "AudioEngine.h"
#include "AnalyzerThread.h"
#include "GraphicsThread.h"
#include "TripleBuffer.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

#define GRUVBOX_BG \
  Color{ 40, 40, 40, 255 } // #282828

#include "raylib.h"

int main()
{
	std::string filePath = "demos/audio11.wav";

	// triple-buffer pattern for safe-concurrent access between visualizer and analyzer
	TripleBuffer share_ag{};

	// SPSC lock-free ring buffer used by audio callback and analyzer
	RingBuffer ringBuffer = RingBuffer();

	AudioEngine audioObj = AudioEngine(ringBuffer, filePath);

	// launched as a functor in its overloaded operator()
	AnalyzerThread analyzerThread = AnalyzerThread(std::ref(ringBuffer), std::ref(share_ag));
	analyzerThread.Launch(); 
	
	if (!audioObj.Init())
	{
		return EXIT_FAILURE;
	}
	audioObj.Start();

	GraphicsThread visualizer{ 1280,720, share_ag };
	visualizer.Initialize(); // initialize window and constants

	while (!WindowShouldClose())
	{
		visualizer.Swap();
		BeginDrawing();
		visualizer.Draw();
		EndDrawing();
	}

	return EXIT_SUCCESS;
}