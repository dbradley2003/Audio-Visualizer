#include "AnalyzerThread.h"
#include "AudioEngine.h"
#include "GraphicsThread.h"
#include "RingBuffer.h"
#include "TripleBuffer.h"
#include "constants.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720



std::atomic_bool doneFlag(false);
int main() {
  // select to play any file in demos directory
  std::string filePath = "demos/audio4.wav";
  TripleBuffer<std::vector<float>> tripleBuffer(BIN_COUNT);

  // SPSC lock-free ring buffer used by audio callback and analyzer

  RingBuffer sharedRingBuffer;

  AudioEngine audioObj(sharedRingBuffer, filePath);

  // launched as a functor in its overloaded operator()
  AnalyzerThread analyzerThread(std::ref(sharedRingBuffer),
                                std::ref(tripleBuffer), std::ref(doneFlag));

  analyzerThread.Launch();
  if (!audioObj.Init()) {
    return EXIT_FAILURE;
  }

  audioObj.Start();

  GraphicsThread visualizer{SCREEN_HEIGHT, SCREEN_WIDTH, tripleBuffer};
  visualizer.Initialize(); // initialize window and constants
  while (!WindowShouldClose()) {
    visualizer.Update();
    BeginDrawing();
    ClearBackground(BLACK);
    visualizer.Draw();
    DrawFPS(10, 10);
    EndDrawing();
  }

  doneFlag.store(true);
  return EXIT_SUCCESS;
}