#ifndef ANALYZER_THREAD_H
#define ANALYZER_THREAD_H

#include <valarray>
#include <complex>
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>

class RingBuffer;
struct AnalyzerGraphicsShare;

typedef std::complex<double> ComplexValue;
typedef std::valarray<ComplexValue> ComplexArray;

class AnalyzerThread
{
public:
	
	static const int SAMPLE_SIZE = 1024;

	//AnalyzerThread(RingBuffer& buffer, std::atomic<std::shared_ptr<std::vector<float>>>& _shared);
	AnalyzerThread(
		RingBuffer& buffer,
		AnalyzerGraphicsShare& share_ag
	);

	
	AnalyzerThread(const AnalyzerThread&) = delete;
	AnalyzerThread(AnalyzerThread&&) = delete;
	AnalyzerThread& operator=(const AnalyzerThread&) = delete;
	AnalyzerThread& operator=(AnalyzerThread&&) = delete;

	void operator()();

	void Update();
	void fft(ComplexArray& data);
	void GetSamples();

private:
	RingBuffer& buffer;
	std::vector<ComplexValue> samples;
	std::vector<float> m_hannTable;

	std::shared_ptr<std::vector<float>> current;
	ComplexArray data;

	AnalyzerGraphicsShare& share_ag;
};

#endif
