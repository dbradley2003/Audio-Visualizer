#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>
#include "raylib.h"
#include "TripleBuffer.h"

struct AnalyzerGraphicsShare;

class GraphicsThread {
public:
	static constexpr int BUCKET_COUNT = 64;
	static constexpr int BAR_SPACING = 5;
	
	GraphicsThread(int screenHeight, int screenWidth, TripleBuffer<std::vector<float>>& share_ag);
	GraphicsThread(const GraphicsThread&) = delete;
	GraphicsThread(GraphicsThread&&) = delete;
	GraphicsThread& operator=(const GraphicsThread&) = delete;
	GraphicsThread& operator=(GraphicsThread&&) = delete;
	~GraphicsThread();

	void Initialize();
	void Draw();
	bool Swap();
	void fftProcess(std::shared_ptr<std::vector<float>>&);
private:
	std::shared_ptr<std::vector<float>> readBuffer;
	std::vector<float> visualHeights;
	std::vector<float> smoothState;
	std::vector<float> smearedState;

	TripleBuffer<std::vector<float>>& share_ag;

	//constants
	int screenWidth;
	int screenHeight;
	float halfWidth;
	RenderTexture2D target;
	const float SMOOTHNESS{ 30.0f };
	const float SMEAREDNESS{ 5.0f };
};

#endif
