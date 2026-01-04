#include <iostream>
#include <thread>

#include "RingBuffer.h"
#include "AudioEngine.h"
#include "AnalyzerThread.h"
#include "GraphicsThread.h"
#include "AnalyzerGraphicsShare.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

/*
TODO:
	* Logic to cleanly shut down all threads and program (JoinThreads struct)
	* Advanced visualizations for spectrum audio data. For example:
		* youtube.com/watch?v=B3DfVHGuvoY(Procedural Mesh Generation), 
		* www.youtube.com/watch?v=XSJDIVazepM (Dynamic Primitives)
	* Window UI:
		* Button for pause/play
		* Ability to select songs
		* Button to exit program
		* Display audio visualization
*/

int main()
{
	RingBuffer ringBuffer = RingBuffer();
	AudioEngine engine = AudioEngine(ringBuffer);

	AnalyzerGraphicsShare share_ag;
	AnalyzerThread analyzerThread = AnalyzerThread(std::ref(ringBuffer), std::ref(share_ag));
	GraphicsThread graphicsThread = GraphicsThread(std::ref(share_ag));
	
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