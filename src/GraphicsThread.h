#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include "Drawable.h"
#include "ParticleGenerator.h"
#include "TripleBuffer.h"
#include "constants.h"
#include "raylib.h"
#include <memory>
#include <vector>

using namespace Constants;

class GraphicsThread {
public:
  GraphicsThread(int screenHeight, int screenWidth,
                 TripleBuffer<std::vector<float>> &sharedBuffer);
  GraphicsThread(const GraphicsThread &) = delete;
  GraphicsThread(GraphicsThread &&) = delete;
  GraphicsThread &operator=(const GraphicsThread &) = delete;
  GraphicsThread &operator=(GraphicsThread &&) = delete;
  ~GraphicsThread();

  void Initialize();
  void Update();
  void Draw();

private:
  void prepareVisuals();
  void fftProcess();
  bool Swap();
  void DrawGridLines() const;
  void DrawVisualBars() const;
  void ScreenShake();
  void PrecomputeGradient();

  // Dimensions
  int screenHeight{0};
  int screenWidth{0};

  // Core Structures
  TripleBuffer<std::vector<float>> &share_ag;
  std::unique_ptr<std::vector<float>> readBuffer;
  std::vector<float> smoothState;
  std::vector<float> smearedState;
  std::vector<Drawable<>> visBars;
  std::array<Color, 256> colorLUT;

  // Scene Variables
  float barWidth{0.0f};
  float halfWidth{0.0f};
  RenderTexture2D target;
  ParticleGenerator particleGenerator;

  // Control Variables
  float mScreenTrauma{0.0f};
};

#endif
