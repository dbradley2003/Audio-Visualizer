#ifndef ANALYZER_THREAD_H
#define ANALYZER_THREAD_H

#include <array>
#include <atomic>
#include <complex>
#include <memory>
#include <thread>
#include <valarray>
#include <vector>

#include "RingBuffer.h"
#include "TripleBuffer.h"
#include "constants.h"

typedef std::complex<double> ComplexValue;
typedef std::valarray<ComplexValue> ComplexArray;

class AnalyzerThread {
public:
  AnalyzerThread(RingBuffer &inputQueue,
                 TripleBuffer<std::vector<float>> &swapLocation,
                 std::atomic<bool> &doneFlag);
  AnalyzerThread(const AnalyzerThread &) = delete;
  AnalyzerThread(AnalyzerThread &&) = delete;
  AnalyzerThread &operator=(const AnalyzerThread &) = delete;
  AnalyzerThread &operator=(AnalyzerThread &&) = delete;
  ~AnalyzerThread();

  void operator()();
  void Launch();


private:
  void fft(ComplexArray &data);
  void GetSamples();
  void Initialize();
  void ApplyHanning();
  void Update();

  std::array<ComplexValue, Constants::FFT_SIZE> samples = {0};
  std::array<float, Constants::FFT_SIZE> mHannTable = {0.0f};

  ComplexArray fftData;
  RingBuffer &inputQueue;
  TripleBuffer<std::vector<float>> &swapLocation;
  std::unique_ptr<std::vector<float>> buckets;
  std::thread mThread;
  std::atomic<bool> &doneFlag;
};

#endif
