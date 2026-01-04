#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

struct AnalyzerGraphicsShare;

class GraphicsThread {
public:
	static const int SAMPLE_SIZE = 1024;

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
