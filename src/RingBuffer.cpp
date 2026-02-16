#include "RingBuffer.h"
#include <iostream>

size_t RingBuffer::GetAvailable() const {
  return mask(this->write.load() - this->read.load());
}
bool RingBuffer::PopFront(float &val) {
  if (this->nextRead == this->localWrite) {
    const size_t actualWrite = this->write.load(std::memory_order_acquire);
    if (this->nextRead == actualWrite) {
      return false;
    }

    localWrite = actualWrite;
  }

  val = this->data[this->nextRead];
  this->nextRead = inc(this->nextRead);
  ++this->rBatch;

  if (this->rBatch >= BATCH_SIZE) {
    this->read.store(this->nextRead, std::memory_order_release);
    rBatch = 0;
  }
  return true;
}

bool RingBuffer::PushBack(const float val) {
  const size_t afterNextWrite = inc(this->nextWrite);

  if (afterNextWrite == this->localRead) {
    const size_t actualRead = this->read.load(std::memory_order_acquire);
    if (afterNextWrite == actualRead) {
      return false;
    }
    this->localRead = actualRead;
  }

  this->data[nextWrite] = val;
  this->nextWrite = afterNextWrite;
  ++this->wBatch;

  if (wBatch >= BATCH_SIZE) {
    this->write.store(nextWrite, std::memory_order_release);
    wBatch = 0;
  }
  return true;
}

size_t RingBuffer::inc(const size_t val) const { return mask(val + 1); }

size_t RingBuffer::mask(const size_t val) const {
  return val & (BUFFER_SIZE - 1);
}
