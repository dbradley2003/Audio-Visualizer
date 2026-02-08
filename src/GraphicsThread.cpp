#include "GraphicsThread.h"

#include <cmath>

#include "raylib.h"
#include "raymath.h"
#include "Bar.h"

#pragma warning(disable: 4244)

using namespace std;
using namespace CyberpunkColors;

namespace
{
	class GhostDrawer {
	public:
		GhostDrawer(Color mColor_)
			:mColor(mColor_)
		{
		}
		void operator()(const Bar& bar)
		{
			if (bar.height() <= 2) return;

			Color colTop = ColorAlpha(mColor, 0.05f);
			Color colBot = ColorAlpha(mColor, 0.3f);

			DrawRectangle(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), ColorAlpha(BLACK, 0.5f));
			DrawRectangle(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), ColorAlpha(BLACK, 0.5f));

			DrawRectangleGradientV(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), colTop, colBot);
			DrawRectangleGradientV(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), colTop, colBot);

		};
	private:
		Color mColor;
	};

	class SolidDrawer {
	public:
		SolidDrawer(Color topColor_, Color bottomColor_)
			:
			topColor(topColor_),
			bottomColor(bottomColor_)
		{
		}

		void operator()(const Bar& bar)
		{
			if (bar.height() < 1.0f) return;

			DrawRectangleGradientV(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), topColor, bottomColor);
			DrawRectangleGradientV(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), topColor, bottomColor);

			Color glow = ColorAlpha(topColor, 0.5f);

			DrawRectangle(bar.xLeft(), bar.y() - 2, bar.width(), 4, glow);
			DrawRectangle(bar.xRight(), bar.y() - 2, bar.width(), 4, glow);

			DrawRectangle(bar.xLeft(), bar.y(), bar.width(), 2, ColorAlpha(WHITE, 0.9f));
			DrawRectangle(bar.xRight(), bar.y(), bar.width(), 2, ColorAlpha(WHITE, 0.9f));

		};
	private:
		Color topColor;
		Color bottomColor;
	};
}

GraphicsThread::GraphicsThread(int screenWidth_, int screenHeight_, TripleBuffer<std::vector<float>>& share_ag_)
	:
	screenWidth(screenWidth_),
	screenHeight(screenHeight_),
	share_ag(share_ag_),
	smoothState(BUCKET_COUNT, 0.0f),
	smearedState(BUCKET_COUNT, 0.0f),
	mTargetZoom(1.0f)
{
	visBars.reserve(BUCKET_COUNT * 2);
}

void GraphicsThread::Initialize()
{
	SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
	InitWindow(screenWidth, screenHeight, "FFT Visualizer");

	Vector2 scale = GetWindowScaleDPI();
	this->target = LoadRenderTexture(screenWidth * scale.x, screenHeight * scale.y);
	SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

	// this->target = LoadRenderTexture(screenWidth, screenHeight);
	// SetTextureWrap(target.texture, TEXTURE_WRAP_CLAMP);


	this->halfWidth = (float)screenWidth / 2.0f;
	float availableWidth = halfWidth - (BAR_SPACING * BUCKET_COUNT);
	this->barWidth = std::max(std::floor(availableWidth / (float)BUCKET_COUNT), 1.0f);
	this->readBuffer = share_ag.consumerReadBuffer();

	mCamera.zoom = 1.0f;
	mCamera.target = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.offset = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.rotation = 0.0f;

	particleGenerator.Init();
}

bool GraphicsThread::Swap()
{
	return share_ag.swapConsumer(this->readBuffer);
}

void GraphicsThread::Update()
{
	this->Swap();
	if (!this->readBuffer || (*this->readBuffer).empty()) {
		return;
	}

	this->fftProcess();
}

void GraphicsThread::fftProcess()
{
	int numInputBins = (int)(*this->readBuffer).size() / 2;
	for (int bar{ 0 }; bar < BUCKET_COUNT; ++bar)
	{
		float tStart = (float)bar / (float)BUCKET_COUNT;
		float tEnd = (float)(bar + 1) / (float)BUCKET_COUNT;

		int fStart = (int)(powf(tStart, 2.5f) * (float)numInputBins);
		int fEnd = (int)(powf(tEnd, 2.5f) * (float)numInputBins);

		if (fStart < 2) fStart = 2;
		if (fEnd <= fStart) fEnd = fStart + 1;
		if (fEnd > numInputBins) fEnd = numInputBins;

		float sum = 0.0f;
		int count = 0;

		for (int q = fStart; q < fEnd; ++q)
		{
			sum += (*readBuffer)[q];
			count++;
		}

		float val = (count > 0) ? (sum / count) : 0.0f;
		float dt = GetFrameTime();

		smoothState[bar] += (val - smoothState[bar]) * SMOOTHNESS * dt;
		smearedState[bar] += (smoothState[bar] - smearedState[bar]) * SMEAREDNESS * dt;
	}
}

GraphicsThread::~GraphicsThread()
{
	UnloadRenderTexture(target);
}

void GraphicsThread::prepareVisuals()
{
	const int centerY = screenHeight / 2;
	const int size = static_cast<int>(smoothState.size());
	const int maxBin = std::min(size, 20);
	const float TREBLE_MULT = 1.5f;

	float bass = smoothState.empty() ? 0.0f : smoothState[0];
	float bassShock = bass * bass * bass;
	float treble = 0.0f;

	if (!smoothState.empty())
	{
		int count = 0;
		for (int i = 5; i < maxBin; ++i)
		{
			treble += smoothState[i];
			++count;
		}
		if (count > 0) treble /= count;
		treble *= TREBLE_MULT;
	}

	particleGenerator.update(bass, treble);

	const Color ghostBase = { 0,220,255,255 };
	const Color ghostDrop = { 255,60,0,255 };
	const Color ghostColor = ColorLerp(ghostBase, ghostDrop, bassShock);

	for (int i{ 0 }; i < BUCKET_COUNT; ++i)
	{
		float height = smoothState[i] * screenHeight;

		int ghostHeight = static_cast<int>(smearedState[i] * screenHeight);
		int mainHeight = static_cast<int>(smoothState[i] * screenHeight);
		int x = i * (barWidth + BAR_SPACING);
		int xRight = static_cast<int>(halfWidth + x);
		int xLeft = static_cast<int>(halfWidth - x - barWidth);
		int ghostY = centerY - (ghostHeight / 2);
		int mainY = centerY - (mainHeight / 2);

		Color normalTop = BLACK;
		Color normalBot = DARK_CYAN;

		if (height < 0.8f)
		{
			normalTop = ColorLerp(BLACK, NEON_CYAN, height * 1.25f);
		}
		else
		{
			float t = (height - 0.8f) * 5.0f;
			normalTop = ColorLerp(NEON_CYAN, ICE_BLUE, t);
			if (height > 0.9f) normalTop = ColorLerp(normalTop, WHITE, (height - 0.9f) * 10.0f);
		}

		Color dropTop = { 40,0,0,255 };
		Color dropBot = { 255,60,0,255 };

		if (bassShock > 0.8f) {
			dropTop = ColorLerp(dropTop, { 255,255,200,255 }, (bassShock - 0.8f) * 5.0f);
		}

		Color finalTop = ColorLerp(normalTop, dropTop, bassShock);
		Color finalBot = ColorLerp(normalBot, dropBot, bassShock);

		auto sd = [=](const Bar& bar) {
			SolidDrawer sd(finalTop, finalBot);
			sd(bar);
			};

		auto gd = [=](const Bar& bar) {
			GhostDrawer gd(ColorAlpha(ghostColor, 0.25f));
			gd(bar);
			};

		visBars.emplace_back(Bar{ mainHeight, (int)barWidth, xLeft, xRight, mainY }, sd);
		visBars.emplace_back(Bar{ ghostHeight, (int)barWidth, xLeft, xRight, ghostY }, gd);
	};
}

void GraphicsThread::DrawGridLines()
{
	BeginBlendMode(BLEND_ADDITIVE);
	const float horizonY = screenHeight / 2.0f;
	const float centerX = screenWidth / 2.0f;
	const float time = (float)GetTime();

	// Draw horizontal lines
	for (int i = 0; i < 25; ++i)
	{
		float movement = fmodf(time * 100.0f, 100.0f);

		float dist = (i * i * 2.5f);
		if (dist + horizonY > screenHeight) break;

		float y = horizonY + dist + (movement * (i * 0.05f));

		float alpha = fminf(1.0f, i / 5.0f) * 0.3f;
		DrawLine(-200, y, screenWidth + 200, y, ColorAlpha(GRID_COLOR, alpha));
	}

	// Draw vertical lines
	for (int i = -10; i <= 10; ++i)
	{
		if (i == 0) continue;

		float spread = i * (screenWidth * 0.15f);

		DrawLine(centerX, horizonY, centerX + spread, screenHeight, GRID_COLOR);
	}

	DrawLine(0, horizonY, screenWidth, horizonY, NEON_CYAN);
	DrawLine(0, horizonY + 1, screenWidth, horizonY + 1, ColorAlpha(NEON_CYAN, 0.5f));
	EndBlendMode();
}

void GraphicsThread::ScreenShake()
{
	const float dt = GetFrameTime();
	const float time = GetTime();
	const float bassLimit = 0.7f;
	const float shakeMult = 1.5f;
	const float shakeLimit = 0.7f;
	const float decaySpeed = 5.0f;

	static float shakeEnergy = 0.0f;
	float bass = smoothState.empty() ? 0.0f : smoothState[0];

	if (bass > bassLimit)
	{
		float hitStrength = (bass - bassLimit) / 0.3f;
		shakeEnergy += hitStrength * 0.1f;
	}

	shakeEnergy -= decaySpeed * dt;
	if (shakeEnergy < 0.0f) shakeEnergy = 0.0f;

	const Rectangle srcRec = { 0,0, (float)target.texture.width, (float)-target.texture.height };
	const Vector2 origin = { 0, 0 };

	float rawShake = shakeEnergy * shakeEnergy * shakeMult;
	if (rawShake > shakeLimit) rawShake = shakeLimit;

	const float shakePower = rawShake;
	const float shakeX = sinf(time * 30.0f) * shakePower;
	const float shakeY = cosf(time * 20.0f) * (shakePower * 0.5f);

	// Draw to shakeX and shakeY
	BeginBlendMode(BLEND_ALPHA);
	DrawTexturePro(target.texture, srcRec,
		{ shakeX, shakeY, (float)screenWidth, (float)screenHeight },
		origin, 0.0f, WHITE);
	EndBlendMode();
}

void GraphicsThread::DrawVisualBars()
{
	BeginBlendMode(BLEND_ADDITIVE);
	for (auto& visual : this->visBars)
	{
		draw(visual);
	}
	EndBlendMode();
}

void GraphicsThread::Draw()
{
	// Clear and reserve memory for visual bars before drawing
	visBars.clear();
	visBars.reserve(BUCKET_COUNT * 2);
	this->prepareVisuals();

	static float intensity = 0.0f;

	// Constants
	const float dt = GetFrameTime();
	const Color bgColor = { 2, 2, 10, 255 };
	const Color transparentColor = Color{ 0,0,0,70 };
	const float bassThreshold = 0.6f;
	const float attackSpeed = std::min(5.0f * dt, 1.0f);
	const float decaySpeed = std::min(1.5f * dt, 1.0f);

	float rawBass = smoothState.empty() ? 0.0f : smoothState[0];
	if (std::isnan(rawBass)) rawBass = 0.0f;
	const float bass = std::clamp(rawBass, 0.0f, 1.0f);

	if (bass > bassThreshold) intensity += (bass - bassThreshold) * 3.0f * dt;

	intensity -= 2.0f * dt;
	intensity = std::clamp(intensity, 0.0f, 1.0f);

	const float targetZoom = 1.0f + (intensity * 0.05f);
	const float maxAngle = 1.0f + (intensity * 4.0f);

	if (targetZoom > mCamera.zoom) mCamera.zoom = Lerp(mCamera.zoom, targetZoom, attackSpeed);
	else mCamera.zoom = Lerp(mCamera.zoom, targetZoom, decaySpeed);

	mCamera.zoom = std::clamp(mCamera.zoom, 0.5f, 1.15f);
	mCamera.rotation = sinf((float)GetTime() * 1.0f) * maxAngle;

	BeginTextureMode(target);
	BeginBlendMode(BLEND_ALPHA);

	DrawRectangle(0, 0, target.texture.width, target.texture.height, transparentColor);
	EndBlendMode();

	BeginMode2D(mCamera);

	// Create glowing flash
	if (intensity > bassThreshold)
	{
		BeginBlendMode(BLEND_ADDITIVE);
		Color centerCol = ColorAlpha(CyberpunkColors::NEON_ORANGE, intensity * 0.5);
		Color edgeCol = ColorAlpha(BLACK, 0.0f);
		DrawCircleGradient(screenWidth / 2, screenHeight / 2, screenWidth * 0.8f, centerCol, edgeCol);
		EndBlendMode();
	}

	this->DrawGridLines();
	particleGenerator.Draw();
	this->DrawVisualBars();

	EndMode2D();
	EndTextureMode();

	//BeginDrawing();
	ClearBackground(bgColor);
	this->ScreenShake();

	Rectangle source = {0, 0, (float)target.texture.width, -(float)target.texture.height};
	Rectangle dest = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
	Vector2 origin = {0, 0};

	DrawTexturePro(target.texture, source, dest, origin, 0.0f, WHITE);
	//EndDrawing();
}
