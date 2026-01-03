#include "GraphicsThread.h"

#include "raylib.h"
#include <atomic>
#include <iostream>
#include <algorithm>

void DrawNeonBar(int x, int y, int width, int height, Color color)
{
	Color glowColor = color;
	glowColor.a = 25;
	DrawRectangle(x - 4, y, width + 8, height, glowColor);

	glowColor.a = 10;
	DrawRectangle(x - 2, y, width + 4, height, glowColor);

	Color coreColor = ColorBrightness(color, 0.1f);
	DrawRectangle(x, y, width, height, coreColor);
}

//GraphicsThread::GraphicsThread(std::atomic<std::shared_ptr<std::vector<float>>>& _shared)
//	:shared(_shared)
//{
//
//}

GraphicsThread::GraphicsThread(std::shared_ptr<std::vector<float>>& shared,
	std::atomic<bool>& dataReady, std::mutex& mtx)
	:mailbox(shared),
	dataReady(dataReady),
	mtx(mtx)
{
	this->readBuffer = std::make_shared<std::vector<float>>(SAMPLE_SIZE / 2);
}

void VerifyIntegrity(const std::vector<float>& data)
{
	if (data.empty())
	{
		return;
	}

	float first = data[0];
	for (size_t i = 1; i < data.size(); ++i)
	{
		if (data[i] != first)
		{
			std::cerr << "CRITICAL FAILURE: Tearing detected at index " << i
				<< ". Expected " << first << " got " << data[i] << std::endl;
			std::cin.get();
		}
	}
}

void GraphicsThread::operator()()
{
	const int screenWidth = 1280;
	const int screenHeight = 720;
	InitWindow(screenWidth, screenHeight, "FFT Visualizer");
	SetTargetFPS(60);
	std::cout << "Graphics Thread Initialized" << std::endl;

	for (int i{}; i < screenWidth; i += 40)
	{
		DrawLine(i, 0, i, screenHeight, GetColor(0x111111FF));
	}

	for (int i{}; i < screenHeight; i += 40)
	{
		DrawLine(0, i, screenWidth, i, GetColor(0x111111FF));
	}




	while (!WindowShouldClose())
	{

		std::unique_lock<std::mutex> lck(this->mtx);
		if (lck && dataReady.load())
		{
			std::swap(this->mailbox, this->readBuffer);
			this->dataReady = false;
		}
		lck.unlock();


		//VerifyIntegrity(*this->readBuffer);
		BeginDrawing();
		this->Draw(GetScreenWidth(), GetScreenHeight());
		EndDrawing();
	}
}

void DrawNeonBar(int x, int y, int w, int h, Color col, float bass) {
	if (h <= 0 || w <= 0) return;

	float flicker = (float)GetRandomValue(90, 100) / 100.0f;
	float glowAlpha = (0.1f + (bass * 0.4f)) * flicker;

	DrawRectangle(x - 3, y, w + 6, h, ColorAlpha(col, glowAlpha));

	DrawRectangle(x, y, w, h, col);

	if (w > 2) {
		DrawRectangle(x + (w / 2), y, 1, h, Color{ 255, 255, 255, 220 });
	}
}

void GraphicsThread::Draw(int screenWidth, int screenHeight)
{

	auto buff = this->readBuffer;//std::atomic_load(&shared);
	if (!buff || buff->empty())
	{
		return;
	}

	size_t N = buff->size();
	if (this->visualHeights.size() != N) this->visualHeights.resize(N, 0.0f);

	float halfWidth = (float)screenWidth / 2.0f;
	int barSpacing = 1;
	int activeBins = (int)(N - 2);
	float barWidth = (halfWidth / (float)activeBins) - barSpacing;
	if (barWidth < 1.0f) barWidth = 1.0f;

	float rawBass = (*buff)[2];
	BeginBlendMode(BLEND_ADDITIVE);
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color({ 10, 0, 20, 20 }));

	if (rawBass > 0.6f)
	{
		Color pulseColor = ColorFromHSV(190.0f, 1.0f, 1.0f);
		DrawCircleGradient(screenWidth / 2, screenHeight / 2,
			rawBass * 400.0f,
			ColorAlpha(pulseColor, 0.15f * rawBass),
			BLANK);
	}

	for (int i = 2; i < N; ++i)
	{
		float magnitude = (*buff)[i];

		if (i < 15)
		{
			float taper = (float)i / 15.0f;
			magnitude *= (taper * taper);
		}

		float t = (float)(i - 2) / (float)(N - 2);

		float xOffset = (float)(i - 2) * (barWidth + barSpacing);

		int xRight = (int)(halfWidth + xOffset);
		int xLeft = (int)(halfWidth - xOffset - barWidth);

		float logMag = log10f(1.0f + magnitude * 9.0f);
		float targetHeight = logMag * screenHeight;

		if (targetHeight > visualHeights[i])
		{
			visualHeights[i] = targetHeight;
		}
		else
		{
			visualHeights[i] -= (screenHeight * 1.5f) * GetFrameTime();
			if (visualHeights[i] < 0) visualHeights[i] = 0;
		}

		int centerY = screenHeight / 2;
		int h = (int)visualHeights[i];
		int y = centerY - (h / 2);

		float curve = powf(t, 2.0f);
		float baseHue = 190.0f + (curve * 130.0f);
		float finalHue = fmodf(baseHue + (float)GetTime() * 15.0f, 360.0f);
		float saturation = 0.8f + (rawBass * 0.2f);
		Color neonColor = ColorFromHSV(finalHue, saturation, 1.0f);

		if (h > 0) {
			if (i == 2)
			{
				DrawNeonBar(xRight, y, (int)barWidth + 1, h, neonColor, rawBass);
				DrawNeonBar(xLeft, y, (int)barWidth + 1, h, neonColor, rawBass);
			}
			else
			{
				DrawNeonBar(xRight, y, (int)barWidth, h, neonColor, rawBass);
				DrawNeonBar(xLeft, y, (int)barWidth, h, neonColor, rawBass);
			}
		}
	}
	EndBlendMode();
	BeginBlendMode(BLEND_MULTIPLIED);
	DrawCircleGradient(screenWidth / 2, screenHeight / 2,
		screenWidth / 1.2f, BLANK, Color{ 0,0,0,200 });
	EndBlendMode();
}

GraphicsThread::~GraphicsThread()
{

}