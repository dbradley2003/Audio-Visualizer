#ifndef TRIPLE_BUFFER_H
#define TRIPLE_BUFFER_H

#include <mutex>
#include <vector>
#include <atomic>
#include <memory>

/*
	Triple Buffer for Minimal-Contention Thread Communication

	Problem with Double Buffering:
	In a standard double-buffer, if the Analyzer (Producer) finishes processing a frame while the
	Visualizer (Consumer) is still drawing the previous frame, the Analyzer either blocks (waits) or
	overrides the data currently being read by the Visualizer.

	Triple Buffer Logic:
	* Analyzer (Thread A) works on private 'Write Buffer' (No Lock).
	* Visualizer (Thread B) works on private 'Read Buffer' (No Lock).
	* Synchronization required on 'Staging Buffer', done by a mutex (Fine Grained Locking).
	
	Swap Logic (Critical Section):
	* The Analyzer finishes processing the data and performs Swap (Staging <---> Write).
	* When the Visualizer finishes drawing and performs Swap (Staging <---> Read).
	* No resource contention, only adding a ns delay inside Swap (Critical Section).
*/

template<typename T>
class TripleBuffer
{
public:

	static constexpr int NUM_FREQ_BINS = 512;

	TripleBuffer()
		:
		mtx(),
		newDataReady(false),
		sharedBuffer(std::make_shared<T>(NUM_FREQ_BINS)),
		producerWriteBuffer_(std::make_shared<T>(NUM_FREQ_BINS)),
		consumerReadBuffer_(std::make_shared<T>(NUM_FREQ_BINS))
	{}

	std::shared_ptr<T> producerWriteBuffer()
	{
		return std::move(this->producerWriteBuffer_);
	}

	std::shared_ptr<T> consumerReadBuffer()
	{
		return std::move(this->consumerReadBuffer_);
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
	std::shared_ptr<T> producerWriteBuffer_;
	std::shared_ptr<T> consumerReadBuffer_;
};

#endif
