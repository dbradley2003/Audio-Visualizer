#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>

#include "raylib.h"
#include "TripleBuffer.h"
#include "Bar.h"
#include "Drawable.h"
#include "ParticleGenerator.h"
#include "constants.h"

using namespace Constants;

class GraphicsThread {
public:
	GraphicsThread(int screenHeight, int screenWidth, TripleBuffer<std::vector<float>>& share_ag);
	GraphicsThread(const GraphicsThread&) = delete;
	GraphicsThread(GraphicsThread&&) = delete;
	GraphicsThread& operator=(const GraphicsThread&) = delete;
	GraphicsThread& operator=(GraphicsThread&&) = delete;
	~GraphicsThread();

	void Initialize();
	void Update();
	
	
private:
	void prepareVisuals();
	void fftProcess();
	void Draw();
	bool Swap();

	void DrawGridLines();
	void DrawTex();

	// Audio Data
	std::unique_ptr<std::vector<float>> readBuffer;
	std::vector<float> smoothState;
	std::vector<float> smearedState;
	std::vector<Drawable<>> visBars;
	TripleBuffer<std::vector<float>>& share_ag;
	// Scene
	int screenWidth;
	int screenHeight;
	float halfWidth;
	float barWidth;
	RenderTexture2D target;
	ParticleGenerator particleGenerator;
	Camera2D mCamera;
	float mTargetZoom;
};

#endif
