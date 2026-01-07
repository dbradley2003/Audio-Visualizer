#ifndef ANALYZER_THREAD_H
#define ANALYZER_THREAD_H

#include <valarray>
#include <complex>
#include <vector>
#include <memory>
#include <array>
#include <thread>

#include "TripleBuffer.h"

class RingBuffer;

typedef std::complex<double> ComplexValue;
typedef std::valarray<ComplexValue> ComplexArray;

class AnalyzerThread
{
public:

	static constexpr int SAMPLE_SIZE = 1024;
	static const int HOP_SIZE = 256;

	AnalyzerThread(RingBuffer& buffer, TripleBuffer<std::vector<float>>& share_ag);
	AnalyzerThread(const AnalyzerThread&) = delete;
	AnalyzerThread(AnalyzerThread&&) = delete;
	AnalyzerThread& operator=(const AnalyzerThread&) = delete;
	AnalyzerThread& operator=(AnalyzerThread&&) = delete;
	~AnalyzerThread();

	void operator()();
	void Launch();
	void Update();
	
private:
	void fft(ComplexArray& data);
	void GetSamples();
	void InitHannTable();
	void ApplyHanning();

	RingBuffer& ringBuffer;

	std::array<ComplexValue, SAMPLE_SIZE> samples;
	std::array<float, SAMPLE_SIZE> m_hannTable;
	std::shared_ptr<std::vector<float>> outputBuckets;
	
	ComplexArray fftData;
	TripleBuffer<std::vector<float>>& share_ag;
	std::thread mThread;
};

#endif
