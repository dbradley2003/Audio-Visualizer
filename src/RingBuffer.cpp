#include "RingBuffer.h"
#include <iostream>

RingBuffer::RingBuffer()
	:
	localRead(0),
	localWrite(0),
	nextRead(0),
	nextWrite(0),
	wBatch(0),
	rBatch(0)
{
	for (int i{}; i < BUFFER_CAPACITY; ++i)
	{
		data[i] = 0;
	}
}

int RingBuffer::GetAvailable()
{
	return mask(this->write.load() - this->read.load());
}

bool RingBuffer::PopFront(float& val)
{
	if (this->nextRead == this->localWrite)
	{
		int actualWrite = std::atomic_load_explicit(&this->write, std::memory_order_acquire);
		if(this->nextRead == actualWrite)
		{
			return false;
		}

		localWrite = actualWrite;
	}

	val = this->data[this->nextRead];
	this->nextRead = inc(this->nextRead);
	this->rBatch++;

	if (this->rBatch >= this->batchSize)
	{

		std::atomic_store_explicit(&this->read, this->nextRead, std::memory_order_release);
		rBatch = 0;
	}
	return true;
}

bool RingBuffer::PushBack(float val)
{
	int afterNextWrite = inc(this->nextWrite);

	if (afterNextWrite == this->localRead)
	{
		int actualRead = this->read.load(std::memory_order_acquire);
		if (afterNextWrite == actualRead)
		{
			return false;
		}
		this->localRead = actualRead;
	}

	this->data[nextWrite] = val;
	this->nextWrite = afterNextWrite;
	wBatch++;

	if (wBatch >= this->batchSize)
	{
		std::atomic_store_explicit(&this->write, nextWrite, std::memory_order_release);
		wBatch = 0;
	}
	return true;
}

int RingBuffer::inc(int val)
{
	return mask(val + 1);
}

int RingBuffer::mask(int val)
{
	return val & (BUFFER_CAPACITY - 1);
}