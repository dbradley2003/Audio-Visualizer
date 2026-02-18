#include <algorithm>
#include <cmath>

#include "AnalyzerThread.h"
#include "RingBuffer.h"
#include <iostream>
#include <cassert>
using namespace std;
using namespace Constants;

AnalyzerThread::AnalyzerThread(RingBuffer &inputQueue,
                               TripleBuffer<vector<float>> &swapLocation,
                               atomic<bool> &doneFlag)
    : inputQueue(inputQueue), swapLocation(swapLocation), doneFlag(doneFlag),
      buckets(swapLocation.producerWriteBuffer()) {
  this->Initialize();
}

void AnalyzerThread::Initialize() {
  for (int i = 0; i < FFT_SIZE; ++i) {
    this->mHannTable[i] =
        0.54f - 0.46f * cosf(2.0f * PI * static_cast<float>(i) /
                             static_cast<float>(FFT_SIZE - 1));
  }
}

// Cooley-Turkey FFT
// (in-place, breadth-first, decimation-in-frequency)
// Taken from rosettacode.org
// https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B
void AnalyzerThread::fft(ComplexArray &d) {
  // DFT
  unsigned int N = d.size(), k = N, n;
  double thetaT = 3.14159265358979323846264338328L / N;
  ComplexValue phiT = ComplexValue(cos(thetaT), -sin(thetaT)), T;
  while (k > 1) {
    n = k;
    k >>= 1;
    phiT = phiT * phiT;
    T = 1.0L;
    for (unsigned int l = 0; l < k; l++) {
      for (unsigned int a = l; a < N; a += n) {
        unsigned int b = a + k;
        ComplexValue t = d[a] - d[b];
        d[a] += d[b];
        d[b] = t * T;
      }
      T *= phiT;
    }
  }
  // Decimate
  unsigned int m = (unsigned int)log2(N);
  for (unsigned int a = 0; a < N; a++) {
    unsigned int b = a;
    // Reverse bits
    b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
    b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
    b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
    b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
    b = ((b >> 16) | (b << 16)) >> (32 - m);
    if (b > a) {
      ComplexValue t = d[a];
      d[a] = d[b];
      d[b] = t;
    }
  }
  // Normalize
  ComplexValue f = 1.0 / sqrt(N);
  for (unsigned int i = 0; i < N; i++)
    d[i] *= f;
}

void AnalyzerThread::operator()() {
  while (!doneFlag) {
    Update();
  }
}

void AnalyzerThread::Launch() { this->mThread = std::thread(std::ref(*this)); }

AnalyzerThread::~AnalyzerThread() {
  if (mThread.joinable()) {
    mThread.join();
  }
}

void AnalyzerThread::GetSamples() {
  if (inputQueue.GetAvailable() >= HOP_SIZE) {
    // move window HOP_SIZE items over
    std::copy(this->samples.begin() + HOP_SIZE, this->samples.end(),
              this->samples.begin());

    constexpr size_t writeIndex = FFT_SIZE - HOP_SIZE;
    float val;
    for (int i = 0; i < HOP_SIZE; ++i) {
      inputQueue.PopFront(val);
      this->samples[writeIndex + i] = {val, 0};
    }
  }
}

void AnalyzerThread::ApplyHanning() {
  for (int i = 0; i < FFT_SIZE; ++i) {
    this->fftData[i] *= this->mHannTable[i];
  }
}

void AnalyzerThread::Update() {
  this->GetSamples();
  this->fftData = ComplexArray(this->samples.data(), FFT_SIZE);
  this->ApplyHanning();
  this->fft(this->fftData);

  constexpr float dbFloor = 60.0f;
  constexpr float invRange = 1.0f / dbFloor;
  constexpr float dbAdd = dbFloor;

  const size_t binCount = this->fftData.size() / 2;
  for (int i = 0; i < binCount; ++i) {
    const auto squaredMag = static_cast<float>(std::norm(this->fftData[i]));
    const float db = 10.0f * log10f(squaredMag + 1e-12f);
    float normalized = (db + dbAdd) * invRange;
    (*this->buckets)[i] = std::clamp(normalized, 0.0f, 1.0f);
  }

  this->swapLocation.swapProducer(this->buckets);
}