#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>

struct AnalyzerGraphicsShare;

class GraphicsThread {
public:
	static constexpr int READ_BUFFER_SIZE = 1024 / 2;

	GraphicsThread(AnalyzerGraphicsShare& share_ag);
	GraphicsThread(const GraphicsThread&) = delete;
	GraphicsThread(GraphicsThread&&) = delete;
	GraphicsThread& operator=(const GraphicsThread&) = delete;
	GraphicsThread& operator=(GraphicsThread&&) = delete;
	~GraphicsThread();

	void Draw(int screenWidth, int screenHeight);
	void operator()();
private:
	std::shared_ptr<std::vector<float>> readBuffer;
	std::vector<float> visualHeights;
	AnalyzerGraphicsShare& share_ag;
};

#endif
