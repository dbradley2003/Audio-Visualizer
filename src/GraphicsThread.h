#ifndef GRAPHICS_THREAD_H
#define GRAPHICS_THREAD_H

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

class GraphicsThread {
public:
	static const int SAMPLE_SIZE = 1024;
	
	//GraphicsThread(std::atomic<std::shared_ptr<std::vector<float>>>& _shared);
	
	GraphicsThread(std::shared_ptr<std::vector<float>>& shared,
		std::atomic<bool>&dataReady,std::mutex& mtx);


	GraphicsThread(const GraphicsThread&) = delete;
	GraphicsThread(GraphicsThread&&) = delete;
	GraphicsThread& operator=(const GraphicsThread&) = delete;
	GraphicsThread& operator=(GraphicsThread&&) = delete;
	~GraphicsThread();
	
	void Draw(int screenWidth, int screenHeight);
	void operator()();
private:
	std::atomic<bool>& dataReady;
	std::shared_ptr<std::vector<float>> readBuffer;
	std::shared_ptr<std::vector<float>>& mailbox;
	//std::atomic<std::shared_ptr<std::vector<float>>>& shared;
	std::vector<float> visualHeights;
	std::mutex& mtx;
};

#endif
