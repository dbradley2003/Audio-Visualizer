#include "GraphicsThread.h"

#include "raylib.h"
#include <iostream>
#include <cmath>
#include "rlgl.h"

#pragma warning(disable: 4244)

#define VIS_RED \
  Color{ 251, 73, 52, 255 } // #fb4934

#define VIS_BLUE \
  Color{ 131, 165, 152, 255 } // #83a598

#define VIS_YELLOW \
  Color{ 250, 189, 47, 255 } // #fabd2f

#define VIS_PURPLE \
  Color{ 211, 134, 155, 255 } // #d3869b

using namespace std;

GraphicsThread::GraphicsThread(int screenWidth, int screenHeight, TripleBuffer<std::vector<float>>& share_ag)
	:
	screenWidth(screenWidth),
	screenHeight(screenHeight),
	share_ag(share_ag),
	smoothState(BUCKET_COUNT),
	smearedState(BUCKET_COUNT)
{
	this->readBuffer = share_ag.consumerReadBuffer();
	halfWidth = static_cast<float>((screenWidth / 2.0f));
}

void GraphicsThread::Initialize()
{
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "FFT Visualizer");

	target = LoadRenderTexture(screenWidth, screenHeight);
	SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
}

void DrawNeonBar(int x, int y, int w, int h, Color col, float bass) {
	if (h <= 0 || w <= 0) return;

	float flicker = (float)GetRandomValue(90, 100) / 100.0f;
	float glowAlpha = (0.1f + (bass * 0.8f)) * flicker;

	DrawRectangle(x - 3, y, w + 6, h, ColorAlpha(col, glowAlpha));
	DrawRectangle(x, y, w, h, col);

	if (w > 2) {
		DrawRectangle(x + (w / 2), y, 1, h, VIS_RED);
	}
}

void DrawCoolRectangle(float x, float y, float width, float height, Color color)
{
	DrawRectangle(x, y, width, height, color);     //Use passed color
	 
	DrawRectangleLines(x, y, width, height, ColorAlpha(color, 0.3f)); // Gruvbox foreground
	DrawCircle(x + width / 2, y, width / 4, ColorAlpha(color, 0.2f)); // Aqua as an accent
}

bool GraphicsThread::Swap()
{
	return share_ag.swapConsumer(this->readBuffer);
}

void GraphicsThread::fftProcess(std::shared_ptr<std::vector<float>>& buff)
{
	int numBuckets = BUCKET_COUNT;
	for (int bar{ 0 }; bar < numBuckets; ++bar)
	{
		float tStart = (float)bar / (float)numBuckets;
		float tEnd = (float)(bar + 1) / (float)numBuckets;

		int fStart = (int)(powf(tStart, 2.0f) * 512.0f);
		int fEnd = (int)(powf(tEnd, 2.0f) * 512.0f);

		if (fStart < 2) fStart = 2;
		if (fEnd <= fStart) fEnd = fStart + 1;
		if (fEnd > 512) fEnd = 512;

		float maxInGroup = 0.0f;
		for (int q = fStart; q < fEnd; ++q)
		{
			if ((*buff)[q] > maxInGroup) maxInGroup = (*buff)[q];
		}

		float dt = GetFrameTime();
		smoothState[bar] += (maxInGroup - smoothState[bar]) * SMOOTHNESS * dt;
		smearedState[bar] += (smoothState[bar] - smearedState[bar]) * SMEAREDNESS * dt;
	}
}

GraphicsThread::~GraphicsThread()
{
	UnloadRenderTexture(target);
}

void GraphicsThread::Draw()
{
	this->Swap();

	auto buff = this->readBuffer;
	if (!buff || (*buff).empty())
	{
		return;
	}

	fftProcess(buff);
	size_t n = smoothState.size();

	float barWidth = (halfWidth / (float)64) - BAR_SPACING;
	barWidth = ceilf(barWidth);
	barWidth = std::max(barWidth, 1.0f);
	int centerY = screenHeight / 2;

	BeginTextureMode(target);

	BeginBlendMode(BLEND_ALPHA);
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 5,10,20,30 });
	EndBlendMode();

	BeginBlendMode(BLEND_ADDITIVE);
	for (int i{}; i < (int)n; ++i)
	{
		int mainH = (int)(smoothState[i] * screenHeight);
		int ghostH = (int)(smearedState[i] * screenHeight);
		int yMain = centerY - (mainH / 2);
		int yGhost = centerY - (ghostH / 2);
		float xOffset = i * (barWidth + BAR_SPACING);
		int xRight = static_cast<int>(halfWidth + xOffset);
		int xLeft = static_cast<int>(halfWidth - xOffset - barWidth);

		if (ghostH > 0)
		{
			Color ghostColor = ColorAlpha(VIS_PURPLE, 0.4f);
			DrawRectangle(xRight, yGhost, (int)barWidth, ghostH, ghostColor);
			DrawRectangle(xLeft, yGhost, (int)barWidth, ghostH, ghostColor);
		}
		if (mainH > 0) {
			// Draw the Solid Bar
			
			DrawCoolRectangle(xRight, yMain, (int)barWidth, mainH, VIS_PURPLE);
			DrawCoolRectangle(xLeft, yMain, (int)barWidth, mainH, VIS_PURPLE);

			DrawRectangle(xRight + (int)barWidth / 2, yMain, 2, mainH, WHITE);
			DrawRectangle(xLeft + (int)barWidth / 2, yMain, 2, mainH, WHITE);
		}
	}
	EndBlendMode();

	BeginBlendMode(BLEND_MULTIPLIED);
	DrawCircleGradient(screenWidth / 2, screenHeight / 2,
		screenWidth * 0.8f, BLANK, VIS_PURPLE);
	EndBlendMode();

	EndTextureMode();

	BeginDrawing();
	ClearBackground(BLACK);

	float shake = smoothState[0] * 15.0f;
	Rectangle srcRec = { 0,0, (float)target.texture.width, (float)-target.texture.height };
	Rectangle destRec = { 0, 0, (float)screenWidth, (float)screenHeight };
	Vector2 origin = { 0, 0 };

	BeginBlendMode(BLEND_ADDITIVE);
	DrawTexturePro(target.texture, srcRec,
		{ -shake,0,(float)screenWidth, (float)screenHeight },
		origin, 0.0f, RED);
	DrawTexturePro(target.texture, srcRec,
		{ shake,0,(float)screenWidth, (float)screenHeight },
		origin, 0.0f, BLUE);

	DrawTexturePro(target.texture, srcRec, destRec, origin, 0.0f, GREEN);
	EndBlendMode();

	DrawFPS(10, 10);
	EndDrawing();
}
