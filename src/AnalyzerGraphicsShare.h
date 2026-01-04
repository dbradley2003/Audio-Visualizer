#ifndef ANALYZER_GRAPHICS_SHARE_H
#define ANALYZER_GRAPHICS_SHARE_H

#include <mutex>
#include <vector>
#include <atomic>
#include <memory>

struct AnalyzerGraphicsShare
{
	static constexpr int BUFFER_SIZE = 1024 / 2;
	
	AnalyzerGraphicsShare()
		:
		mtx(),
		newDataReady(false),
		mailbox(std::make_shared<std::vector<float>>(BUFFER_SIZE))
	{
	}
	
	void swapProducer(std::shared_ptr<std::vector<float>>& producerWriteBuffer)
	{
		std::lock_guard<std::mutex> lck(mtx);
		std::swap(this->mailbox, producerWriteBuffer);
		newDataReady = true;
	}
	
	bool swapConsumer(std::shared_ptr<std::vector<float>>& consumerReadBuffer)
	{
		std::unique_lock<std::mutex> lck(mtx, std::try_to_lock);
		if (!newDataReady.load() || !lck)
		{
			return false;
		}

		std::swap(consumerReadBuffer, this->mailbox);
		newDataReady = false;
		return true;
	}

	std::mutex mtx;
	std::atomic<bool> newDataReady;
	std::shared_ptr<std::vector<float>> mailbox;
};

#endif
