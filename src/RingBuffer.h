#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <__new/interference_size.h>
#include <array>
#include <atomic>

class RingBuffer {
public:
  constexpr static unsigned BUFFER_SIZE = 1 << 15;
  constexpr static int BATCH_SIZE = 128;

  RingBuffer() = default;
  RingBuffer(const RingBuffer &) = delete;
  RingBuffer(RingBuffer &&) = delete;
  RingBuffer &operator=(RingBuffer &) = delete;
  RingBuffer &operator=(RingBuffer &&) = delete;
  ~RingBuffer() = default;

  bool PushBack(float val);
  bool PopFront(float &val);
  size_t GetAvailable() const;

private:
  size_t inc(size_t val) const;
  size_t mask(size_t val) const;

  alignas(std::hardware_destructive_interference_size) std::atomic<size_t> read{
      0};
  alignas(
      std::hardware_destructive_interference_size) std::atomic<size_t> write{0};

  /*Consumer Local Variables*/
  alignas(std::hardware_destructive_interference_size) size_t localWrite{0};
  size_t nextRead{0};
  size_t rBatch{0};

  /*Producer Local Variables*/
  alignas(std::hardware_destructive_interference_size) size_t localRead{0};
  size_t nextWrite{0};
  size_t wBatch{0};

  /*Constant variables*/
  alignas(std::hardware_destructive_interference_size)
      std::array<float, BUFFER_SIZE> data{};
};
// namespace AudioEngineDetails
#endif
#pragma warning(pop)