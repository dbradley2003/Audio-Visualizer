#ifndef TRIPLE_BUFFER_H
#define TRIPLE_BUFFER_H

#include <mutex>
#include <vector>
#include <atomic>
#include <memory>

class TripleBuffer
{
public:
	TripleBuffer()
		:
		mtx(),
		newDataReady(false),
		sharedBuffer(std::make_shared<std::vector<float>>(512))
	{
	}

	void swapProducer(std::shared_ptr<std::vector<float>>& producerBuff)
	{
		std::lock_guard<std::mutex> lck(mtx);
		std::swap(this->sharedBuffer, producerBuff);
		newDataReady = true;
	}

	bool swapConsumer(std::shared_ptr<std::vector<float>>& consumerBuff)
	{
		std::unique_lock<std::mutex> lck(mtx, std::try_to_lock);
		if (!newDataReady.load() || !lck)
		{
			return false;
		}

		std::swap(consumerBuff, this->sharedBuffer);
		newDataReady = false;
		return true;
	}
private:
	std::mutex mtx;
	std::atomic<bool> newDataReady;
	std::shared_ptr<std::vector<float>> sharedBuffer;
};

#endif
