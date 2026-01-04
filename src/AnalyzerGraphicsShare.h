#pragma once

#include <mutex>
#include <vector>
#include <atomic>
#include <memory>

struct AnalyzerGraphicsShare
{
	static const int SAMPLE_SIZE = 1024;
	
	AnalyzerGraphicsShare()
		:
		mtx(),
		newDataReady(false)
	{
		this->mailbox = std::make_shared<std::vector<float>>(SAMPLE_SIZE / 2);
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
