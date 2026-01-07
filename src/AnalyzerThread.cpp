#include <cmath>
#include <algorithm>
#include <iostream>

#include "AnalyzerThread.h"
#include "RingBuffer.h"

#include "TripleBuffer.h"

#include <vector>

#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)

#define PI 3.14159265358979323846f

AnalyzerThread::AnalyzerThread(RingBuffer& _ringBuffer, TripleBuffer<std::vector<float>>& _share_ag)
	:
	ringBuffer(_ringBuffer),
	share_ag(_share_ag),
	mThread()
{
	this->outputBuckets = this->share_ag.producerWriteBuffer();
	std::fill(m_hannTable.begin(), m_hannTable.end(), 0.0f);
	std::fill(samples.begin(), samples.end(), 0);
	InitHannTable();
}

void AnalyzerThread::InitHannTable()
{
	for (int i{}; i < SAMPLE_SIZE; ++i)
	{
		this->m_hannTable[i] = 0.54 - 0.46 * cosf(2.0f * PI * i / (SAMPLE_SIZE - 1));
	}
}

void AnalyzerThread::fft(ComplexArray& d)
{
	// DFT
	unsigned int N = d.size(), k = N, n;
	double thetaT = 3.14159265358979323846264338328L / N;
	ComplexValue phiT = ComplexValue(cos(thetaT), -sin(thetaT)), T;
	while (k > 1)
	{
		n = k;
		k >>= 1;
		phiT = phiT * phiT;
		T = 1.0L;
		for (unsigned int l = 0; l < k; l++)
		{
			for (unsigned int a = l; a < N; a += n)
			{
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
	for (unsigned int a = 0; a < N; a++)
	{
		unsigned int b = a;
		// Reverse bits
		b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
		b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
		b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
		b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
		b = ((b >> 16) | (b << 16)) >> (32 - m);
		if (b > a)
		{
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

void AnalyzerThread::operator()()
{
	while (true)
	{
		Update();
	}
}

void AnalyzerThread::Launch()
{
	this->mThread = std::thread(std::ref(*this));
}

AnalyzerThread::~AnalyzerThread()
{
	if (mThread.joinable())
	{
		mThread.join();
	}
}

void AnalyzerThread::GetSamples()
{
	float val;
	int available = ringBuffer.GetAvailable();
	if (available > HOP_SIZE) {

		// move window 128 items over
		std::copy(
			this->samples.begin() + HOP_SIZE,
			this->samples.end(),
			this->samples.begin()
		);

		size_t writeIndex = SAMPLE_SIZE - HOP_SIZE;

		for (int i = 0; i < HOP_SIZE; ++i)
		{
			ringBuffer.PopFront(val);
			this->samples[writeIndex + i] = { val, 0 };
		}
	}
}

void AnalyzerThread::ApplyHanning()
{
	for (int i{}; i < SAMPLE_SIZE; ++i)
	{
		this->fftData[i] *= this->m_hannTable[i];
	}
}

void AnalyzerThread::Update()
{
	this->GetSamples();
	this->fftData = ComplexArray(this->samples.data(), SAMPLE_SIZE);
	this->ApplyHanning();
	this->fft(fftData);

	const float invSixety = 1.0f / 60.0f;

	const int numBins = SAMPLE_SIZE / 2;

	for (int i{}; i < numBins; ++i)
	{
		float squaredMag = std::norm(this->fftData[i]);
		float db = 10.0f * log10f(squaredMag + 1e-12f);
		float normalized = (db + 60.0f) * invSixety;
		(*this->outputBuckets)[i] = std::clamp(normalized, 0.0f, 1.0f);
	}

	// Make fresh data available to visualizer
	this->share_ag.swapProducer(this->outputBuckets);
}