#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>
#include "raylib.h"
#include "TripleBuffer.h"
#include "Bar.h"
#include "Drawable.h"

class GraphicsThread {
public:
	static constexpr int BUCKET_COUNT = 64;
	static constexpr int BAR_SPACING = 2;

	GraphicsThread(int screenHeight, int screenWidth, TripleBuffer<std::vector<float>>& share_ag);
	GraphicsThread(const GraphicsThread&) = delete;
	GraphicsThread(GraphicsThread&&) = delete;
	GraphicsThread& operator=(const GraphicsThread&) = delete;
	GraphicsThread& operator=(GraphicsThread&&) = delete;
	~GraphicsThread();

	void Initialize();
	void Draw();
	bool Swap();
	void Update();
	void fftProcess(std::shared_ptr<std::vector<float>>&);
	void prepareVisuals();
private:
	std::shared_ptr<std::vector<float>> readBuffer;
	std::vector<float> smoothState;
	std::vector<float> smearedState;
	std::vector<Drawable<>> visBars;

	TripleBuffer<std::vector<float>>& share_ag;

	//constants
	int screenWidth;
	int screenHeight;
	
	float halfWidth;
	float barWidth;
	RenderTexture2D target;
	Camera2D camera{ 0 };
	const float SMOOTHNESS{ 30.0f };
	const float SMEAREDNESS{ 5.0f };
};

#endif
