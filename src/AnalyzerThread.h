#ifndef ANALYZER_THREAD_H
#define ANALYZER_THREAD_H

#include <valarray>
#include <complex>
#include <vector>
#include <memory>
#include <array>

class RingBuffer;
struct AnalyzerGraphicsShare;

typedef std::complex<double> ComplexValue;
typedef std::valarray<ComplexValue> ComplexArray;

class AnalyzerThread
{
public:

	static constexpr int SAMPLE_SIZE = 1024;
	static constexpr int WRITE_BUFFER_SIZE = SAMPLE_SIZE / 2;

	AnalyzerThread(RingBuffer& buffer, AnalyzerGraphicsShare& share_ag);
	AnalyzerThread(const AnalyzerThread&) = delete;
	AnalyzerThread(AnalyzerThread&&) = delete;
	AnalyzerThread& operator=(const AnalyzerThread&) = delete;
	AnalyzerThread& operator=(AnalyzerThread&&) = delete;
	void operator()();

	void Update();
	void fft(ComplexArray& data);
	void GetSamples();
	void InitHannTable();

private:
	RingBuffer& ringBuffer;
	//std::vector<std::complex<double>> samples;
	std::array<ComplexValue, SAMPLE_SIZE> samples;
	std::array<float, SAMPLE_SIZE> m_hannTable;
	std::shared_ptr<std::vector<float>> outputBuffer;
	ComplexArray data;
	AnalyzerGraphicsShare& share_ag;
};

#endif
