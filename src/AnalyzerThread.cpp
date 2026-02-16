#include <algorithm>
#include <cmath>

#include "AnalyzerThread.h"
#include "RingBuffer.h"

AnalyzerThread::AnalyzerThread(RingBuffer &ringBuffer,
                               TripleBuffer<std::vector<float>> &share_ag,
                               std::atomic<bool> &done)
    : ringBuffer(ringBuffer), share_ag(share_ag), done(done)
{
  this->outputBuckets = this->share_ag.producerWriteBuffer();
  std::fill(m_hannTable.begin(), m_hannTable.end(), 0.0f);
  std::fill(samples.begin(), samples.end(), 0);
  this->InitHannTable();
}

void AnalyzerThread::InitHannTable() {
  for (int i = 0; i < FFT_SIZE; ++i) {
    this->m_hannTable[i] =
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
  while (!done) {
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
  if (ringBuffer.GetAvailable() > HOP_SIZE) {

    // move window HOP_SIZE items over
    std::copy(this->samples.begin() + HOP_SIZE, this->samples.end(),
              this->samples.begin());

    constexpr size_t writeIndex = FFT_SIZE - HOP_SIZE;
    float val;
    for (int i = 0; i < HOP_SIZE; ++i) {
      ringBuffer.PopFront(val);
      this->samples[writeIndex + i] = {val, 0};
    }
  }
}

void AnalyzerThread::ApplyHanning() {
  for (int i{}; i < FFT_SIZE; ++i) {
    this->fftData[i] *= this->m_hannTable[i];
  }
}

void AnalyzerThread::Update() {
  // process raw audio data for spectral analysis
  this->GetSamples();
  this->fftData = ComplexArray(this->samples.data(), FFT_SIZE);
  this->ApplyHanning();
  this->fft(this->fftData);

  constexpr float invSixty = 1.0f / 60.0f;
  constexpr int binCount = FFT_SIZE / 2;

  for (int i{}; i < binCount; ++i) {
    const float squaredMag = static_cast<float>(std::norm(this->fftData[i]));
    const float db = 10.0f * log10f(squaredMag + 1e-12f);
    float normalized = (db + 60.0f) * invSixty;
    (*this->outputBuckets)[i] = std::clamp(normalized, 0.0f, 1.0f);
  }

  // push new data to shared buffer for visualizer reading
  this->share_ag.swapProducer(this->outputBuckets);
}