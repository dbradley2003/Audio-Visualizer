#ifndef ANALYZER_THREAD_H
#define ANALYZER_THREAD_H

#include <valarray>
#include <complex>
#include <vector>
#include <memory>
#include <array>
#include <thread>

class RingBuffer;
struct AnalyzerGraphicsShare;

typedef std::complex<double> ComplexValue;
typedef std::valarray<ComplexValue> ComplexArray;

class AnalyzerThread
{
public:

	static constexpr int SAMPLE_SIZE = 1024;
	static const int HOP_SIZE = 256;
	static constexpr int WRITE_BUFFER_SIZE = SAMPLE_SIZE / 2;

	AnalyzerThread(RingBuffer& buffer, AnalyzerGraphicsShare& share_ag);
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

	RingBuffer& ringBuffer;

	std::array<ComplexValue, SAMPLE_SIZE> samples;
	std::array<float, SAMPLE_SIZE> m_hannTable;
	std::shared_ptr<std::vector<float>> outputBuckets;
	
	ComplexArray fftData;
	AnalyzerGraphicsShare& share_ag;
	std::thread mThread;
};

#endif
