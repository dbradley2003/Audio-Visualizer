#ifndef TRIPLE_BUFFER_H
#define TRIPLE_BUFFER_H

#include <mutex>
#include <vector>
#include <atomic>
#include <memory>

template<typename T>
class TripleBuffer
{
public:

	static constexpr int BUFFER_SIZE = 512;

	TripleBuffer()
		:
		mtx(),
		newDataReady(false),
		sharedBuffer(std::make_shared<T>(BUFFER_SIZE)),
		pBuffer(std::make_shared<T>(BUFFER_SIZE)),
		cBuffer(std::make_shared<T>(BUFFER_SIZE))
	{
	}

	std::shared_ptr<T> producerWriteBuffer()
	{
		return std::move(pBuffer);
	}

	std::shared_ptr<T> consumerReadBuffer()
	{
		return std::move(cBuffer);
	}

	void swapProducer(std::shared_ptr<T>& producerBuff)
	{
		std::lock_guard<std::mutex> lck(mtx);
		std::swap(this->sharedBuffer, producerBuff);
		newDataReady = true;
	}

	bool swapConsumer(std::shared_ptr<T>& consumerBuff)
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

	std::shared_ptr<T> sharedBuffer;
	std::shared_ptr<T> pBuffer;
	std::shared_ptr<T> cBuffer;
};

#endif
