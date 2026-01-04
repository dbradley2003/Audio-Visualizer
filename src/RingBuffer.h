#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>

#pragma warning(push)
#pragma warning(disable : 4324)

class RingBuffer
{
public:
	constexpr static unsigned BUFFER_CAPACITY = 1 << 15;
	constexpr static unsigned CACHE_LINE = 64;

	RingBuffer();
	RingBuffer(const RingBuffer&) = delete;
	RingBuffer(RingBuffer&&) = delete;
	RingBuffer& operator=(RingBuffer&) = delete;
	RingBuffer& operator=(RingBuffer&&) = delete;
	~RingBuffer() = default;

	bool PushBack(float val);
	bool PopFront(float& val);
	int GetAvailable() const;

private:

	int inc(int val) const;
	int mask(int val) const;

	alignas(CACHE_LINE) std::atomic<int> read{ 0 };
	alignas(CACHE_LINE) std::atomic<int> write{ 0 };

	/*Consumer Local Variables*/
	alignas(CACHE_LINE) int localWrite;
	int nextRead;
	int rBatch;

	/*Producer Local Variables*/
	alignas(CACHE_LINE) int localRead;
	int nextWrite;
	int wBatch;

	/*Constant variables*/
	alignas(CACHE_LINE) float data[BUFFER_CAPACITY];
	const int batchSize = 50;
};

#endif
#pragma warning(pop)