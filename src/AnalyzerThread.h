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

using namespace Constants;

class AnalyzerThread {
public:
  AnalyzerThread(RingBuffer &ringBuffer,
                 TripleBuffer<std::vector<float>> &share_ag,
                 std::atomic<bool> &done);
  AnalyzerThread(const AnalyzerThread &) = delete;
  AnalyzerThread(AnalyzerThread &&) = delete;
  AnalyzerThread &operator=(const AnalyzerThread &) = delete;
  AnalyzerThread &operator=(AnalyzerThread &&) = delete;
  ~AnalyzerThread();

  void operator()();
  void Launch();
  void Update();

private:
  void fft(ComplexArray &data);
  void GetSamples();
  void InitHannTable();
  void ApplyHanning();

  RingBuffer &ringBuffer;
  std::array<ComplexValue, FFT_SIZE> samples;
  std::array<float, FFT_SIZE> m_hannTable;
  std::unique_ptr<std::vector<float>> outputBuckets;

  ComplexArray fftData;
  TripleBuffer<std::vector<float>> &share_ag;
  std::thread mThread;
  std::atomic<bool> &done;
};

#endif
