#include "GraphicsThread.h"

#include <cmath>

#include "raylib.h"
#include "raymath.h"
#include "Bar.h"

#pragma warning(disable: 4244)

using namespace std;
void DrawCoolRectangle(float x, float y, float width, float height, Color color);

namespace DrawingDetails
{

	class GhostDrawer
	{
	public:
		GhostDrawer(Color c)
			:color_(c)
		{
		}

		void operator()(const Bar& bar)
		{
			if (bar.height() > 0) {

				Color invisible = ColorAlpha(color_, 0.0f);
				Color visible = ColorAlpha(color_, 0.4f);

				DrawRectangleGradientV(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), visible, invisible);
				DrawRectangleGradientV(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), visible, invisible);
			}
		};
	private:
		Color color_;
	};

	class SolidDrawer
	{
	public:
		SolidDrawer(Color c)
			:color_(c)
		{
		}

		void operator()(const Bar& bar)
		{
			if (bar.height() > 0) {
				DrawCoolRectangle(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), color_);
				DrawCoolRectangle(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), color_);
			}
		};
	private:
		Color color_;
	};
}

GraphicsThread::GraphicsThread(int w, int h, TripleBuffer<std::vector<float>>& share_ag)
	:
	screenWidth(w),
	screenHeight(h),
	share_ag(share_ag),
	smoothState(BUCKET_COUNT, 0.0f),
	smearedState(BUCKET_COUNT, 0.0f),
	mTargetZoom(1.0f),
	particleGenerator()
{
	visBars.reserve(BUCKET_COUNT * 2);
}

void GraphicsThread::Initialize()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
	InitWindow(screenWidth, screenHeight, "FFT Visualizer");
	SetTargetFPS(144);

	SetTextureWrap(target.texture, TEXTURE_WRAP_CLAMP);

	this->halfWidth = (float)screenWidth / 2.0f;
	float availableWidth = halfWidth - (BAR_SPACING * BUCKET_COUNT);
	this->barWidth = std::max(std::floor(availableWidth / (float)BUCKET_COUNT), 1.0f);

	this->target = LoadRenderTexture(screenWidth, screenHeight);
	SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

	this->readBuffer = share_ag.consumerReadBuffer();

	mCamera.zoom = 1.0f;
	mCamera.target = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.offset = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.rotation = 0.0f;

	particleGenerator.Init(500, screenWidth, screenHeight);
}

bool GraphicsThread::Swap()
{
	return share_ag.swapConsumer(this->readBuffer);
}

void GraphicsThread::Update()
{
	this->Swap();
	if (!this->readBuffer || (*this->readBuffer).empty())
	{
		return;
	}
	this->fftProcess();
	this->Draw();
}

void GraphicsThread::fftProcess()
{
	int numInputBins = (int)(*this->readBuffer).size() / 2;
	int numBuckets = BUCKET_COUNT;
	for (int bar{ 0 }; bar < numBuckets; ++bar)
	{
		float tStart = (float)bar / (float)numBuckets;
		float tEnd = (float)(bar + 1) / (float)numBuckets;

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
	int centerY = screenHeight / 2;
	float bass = smoothState.empty() ? 0.0f : smoothState[0];
	float treble = 0.0f;
	int maxBin = std::min((int)smoothState.size(), 20);

	if (!smoothState.empty())
	{
		int count = 0;
		for (int i = 5; i < maxBin; ++i)
		{
			treble += smoothState[i];
			++count;
		}
		if (count > 0) treble /= count;
		treble *= 2.0f;
	}

	particleGenerator.update(bass, treble);

	auto f = [](const ParticleGenerator& pg) {
		pg();
		};

	visBars.emplace_back(std::ref(particleGenerator), f);

	float bassShock = powf(bass, 3.0f);
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
		Color normalBot = CyberpunkColors::DARK_CYAN;


		if (height < 0.8f)
		{
			normalTop = ColorLerp(BLACK, CyberpunkColors::NEON_CYAN, height * 1.25f);
		}
		else
		{
			float t = (height - 0.8f) * 5.0f;
			Color iceBlue = { 180,255,255,255 };
			normalTop = ColorLerp(CyberpunkColors::NEON_CYAN, iceBlue, t);

			if (height > 0.9f) normalTop = ColorLerp(normalTop, WHITE, (height - 0.9f) * 10.0f);
		}

		Color dropTop = {40,0,0,255};
		Color dropBot = { 255,60,0,255};

		if(bassShock > 0.8f)
		{
			dropTop = ColorLerp(dropTop, {255,255,200,255}, (bassShock - 0.8f) * 5.0f);
		}

		Color finalTop = ColorLerp(normalTop, dropTop, bassShock);
		Color finalBot = ColorLerp(normalBot, dropBot, bassShock);

		auto drawSolidBars = [=](const Bar& bar)
			{
				DrawRectangleGradientV(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), finalBot, finalTop);
				DrawRectangleGradientV(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), finalBot, finalTop);

				DrawRectangle(bar.xLeft(), bar.y(), bar.width(), 2, ColorAlpha(WHITE, 0.8f));
				DrawRectangle(bar.xRight(), bar.y(), bar.width(), 2, ColorAlpha(WHITE, 0.8f));
			};
		auto drawGhostBars = [=](const Bar& bar)
			{
				Color ghostBase = { 0,220,255,255 };
				Color ghostDrop = { 255,60,0,255 };

				Color ghostColor = ColorLerp(ghostBase, ghostDrop, bassShock);
				DrawingDetails::GhostDrawer a(ColorAlpha(ghostColor, 0.25f));
				a(bar);
			};

		visBars.emplace_back(Bar{ mainHeight, (int)barWidth, xLeft, xRight, mainY }, drawSolidBars);
		visBars.emplace_back(Bar{ ghostHeight, (int)barWidth, xLeft, xRight, ghostY }, drawGhostBars);
	};
}

void GraphicsThread::DrawGridLines()
{
	BeginBlendMode(BLEND_ADDITIVE);
	Color gridColor = ColorAlpha(Color{ 0,40,50,255 }, 0.25f);
	float time = (float)GetTime();
	for (int i = 0; i < 20; ++i)
	{
		float y = (screenHeight / 2) + (i * i * 2) + (fmodf(time * 20.0f, 20.0f));
		if (y < screenHeight) DrawLine(-200, y, screenWidth + 200, y, gridColor);
	}
	DrawLine(-200, screenHeight / 2, screenWidth + 200, screenHeight / 2, CyberpunkColors::NEON_CYAN);
	EndBlendMode();
}

void GraphicsThread::DrawTex()
{
	float bass = smoothState.empty() ? 0.0f : smoothState[0];
	float dt = GetFrameTime();
	float time = GetTime();
	//float shakeInput = 0.0f;

	static float shakeEnergy = 0.0f;

	if (bass > 0.7f)
	{
		float hitStrength = (bass - 0.7f) / 0.3f;
		shakeEnergy += hitStrength * 0.1f;
		if (shakeEnergy > 1.0f)
		{
			shakeEnergy = 1.0f;
		}
	}

	shakeEnergy -= 3.0f * dt;
	if (shakeEnergy < 0.0f)
	{
		shakeEnergy = 0.0f;
	}

	

	float shakePower = shakeEnergy * shakeEnergy * 4.0f;
	float shakeX = sinf(time * 30.0f) * shakePower;
	float shakeY = cosf(time * 20.0f) * (shakePower * 0.5f);

	Rectangle srcRec = { 0,0, (float)target.texture.width, (float)-target.texture.height };
	Vector2 origin = { 0, 0 };

	BeginBlendMode(BLEND_ALPHA);
	DrawTexturePro(target.texture, srcRec,
		{ shakeX, shakeY, (float)screenWidth, (float)screenHeight },
		origin, 0.0f, WHITE);
	EndBlendMode();
}

void GraphicsThread::Draw()
{
	// clear and reserve memory for visual bars before drawing
	visBars.clear();
	visBars.reserve(BUCKET_COUNT * 2);
	this->prepareVisuals();

	float bass = smoothState.empty() ? 0.0f : smoothState[0];
	if (std::isnan(bass)) bass = 0.0f;
	if (bass < 0.0f) bass = 0.0f;
	if (bass > 1.0f) bass = 1.0f;
	static float intensity = 0.0f;

	float dt = GetFrameTime();

	if (bass > 0.6f)
	{
		intensity += (bass - 0.6f) * 0.05f;
	}

	intensity -= 2.0f * dt;

	if (intensity < 0.0f)
	{
		intensity = 0.0f;
	}
	if (intensity > 1.0f)
	{
		intensity = 1.0f;
	}

	float targetZoom = 1.0f + (intensity * 0.12f);

	float attackSpeed = fminf(4.0f * dt, 1.0f);
	float decaySpeed = fminf(0.5f * dt, 1.0f);

	if (targetZoom > mCamera.zoom)
	{
		mCamera.zoom = Lerp(mCamera.zoom, targetZoom, attackSpeed);
	}
	else
	{
		mCamera.zoom = Lerp(mCamera.zoom, targetZoom, decaySpeed);
	}

	if (mCamera.zoom < 0.5f)
	{
		mCamera.zoom = 0.5f;
	}
	if (mCamera.zoom > 1.2f)
	{
		mCamera.zoom = 1.2f;
	}

	float maxAngle = 1.0f + (intensity * 4.0f);
	mCamera.rotation = sinf((float)GetTime() * 1.0f) * maxAngle;

	BeginTextureMode(target);
	BeginBlendMode(BLEND_ALPHA);
	DrawRectangle(0, 0, target.texture.width, target.texture.height, Color{ 0,0,0,70});
	EndBlendMode();

	BeginMode2D(mCamera);

	if (intensity > 0.6f)
	{
		BeginBlendMode(BLEND_ADDITIVE);
		Color centerCol = ColorAlpha(CyberpunkColors::NEON_ORANGE, intensity * 0.2f);
		Color edgeCol = ColorAlpha(BLACK, 0.0f);
		
		DrawCircleGradient(screenWidth / 2, screenHeight / 2, screenWidth * 0.8f,centerCol,edgeCol);
		
		
		EndBlendMode();
	}

	DrawGridLines();

	BeginBlendMode(BLEND_ADDITIVE);
	for (auto& visual : this->visBars)
	{
		draw(visual);
	}
	EndBlendMode();
	EndMode2D();

	EndTextureMode();

	BeginDrawing();
	ClearBackground(Color{ 2, 2, 10, 255 });
	DrawTex();

	BeginBlendMode(BLEND_MULTIPLIED);
	float vigRadius = screenWidth * (1.3f - (intensity * 0.2f));
	DrawCircleGradient(screenWidth / 2, screenHeight / 2, vigRadius, WHITE, Color{ 80, 80, 80, 255 });
	EndBlendMode();

	DrawFPS(10, 10);
	EndDrawing();
}

void DrawCoolRectangle(float x, float y, float width, float height, Color color)
{

	if (width > 4)
	{
		DrawRectangle(x, y, width + 8, height, ColorAlpha(color, 0.2f));
	}

	Color coreColor = color;

	DrawRectangle(x + 1, y, width - 2, height, ColorAlpha(coreColor, 0.8f));

	if (width > 2)
	{
		DrawRectangle(x + width / 2 - 1, y, 2, height, ColorAlpha(WHITE, 0.5f));
	}

	DrawCircle(x + width / 2, y, width / 2, ColorAlpha(color, 0.4f));
}
