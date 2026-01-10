#include "GraphicsThread.h"

#include "raylib.h"
#include <cmath>
#include "Bar.h"


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
				DrawRectangle(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), color_);
				DrawRectangle(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), color_);
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
				DrawCoolRectangle(bar.xRight(), bar.y(), (int)bar.width(), bar.height(), VIS_PURPLE);
				DrawCoolRectangle(bar.xLeft(), bar.y(), (int)bar.width(), bar.height(), VIS_PURPLE);
				DrawRectangle(bar.xRight() + (int)bar.width() / 2, bar.y(), 2, bar.height(), WHITE);
				DrawRectangle(bar.xLeft() + (int)bar.width() / 2, bar.y(), 2, bar.height(), WHITE);
			}
		};
	private:
		Color color_;
	};
}

GraphicsThread::GraphicsThread(int screenWidth, int screenHeight, TripleBuffer<std::vector<float>>& share_ag)
	:
	screenWidth(screenWidth),
	screenHeight(screenHeight),
	share_ag(share_ag),
	smoothState(BUCKET_COUNT),
	smearedState(BUCKET_COUNT),
	pm(100, screenWidth, screenHeight)
{
}

void GraphicsThread::Initialize()
{
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "FFT Visualizer");

	this->target = LoadRenderTexture(screenWidth, screenHeight);
	SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

	this->readBuffer = share_ag.consumerReadBuffer();

	this->halfWidth = static_cast<float>((screenWidth / 2.0f));
	float tmp = halfWidth / static_cast<float>(BUCKET_COUNT) - BAR_SPACING;
	tmp = std::ceilf(tmp);
	this->barWidth = std::max(tmp, 1.0f);

	mCamera.target = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.offset = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	mCamera.rotation = 0.0f;
	mCamera.zoom = 1.0f;
	mTargetZoom = 1.0f;
}

bool GraphicsThread::Swap()
{
	return share_ag.swapConsumer(this->readBuffer);
}

void GraphicsThread::Update()
{
	this->Swap();

	auto fftData = this->readBuffer;
	if (!fftData || (*fftData).empty())
	{
		return;
	}

	fftProcess(fftData);
	this->Draw();
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
			if ((*buff)[q] > maxInGroup)
			{
				maxInGroup = (*buff)[q];
			}
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

	pm.update(bass, treble);
	auto f = [](const ParticleGenerator& pg) {
		pg();
		};

	visBars.emplace_back(std::ref(pm), f);

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

		Color c;
		if (height < 0.5)
		{
			c = ColorLerp(CyberpunkColors::NEON_CYAN, CyberpunkColors::NEON_PURPLE, height * 2.0f);
		}
		else
		{
			float t = (height - 0.5f) * 2.0f;
			c = ColorLerp(CyberpunkColors::NEON_PURPLE, CyberpunkColors::NEON_PINK, t);

			if (height > 0.9f) c = ColorLerp(c, WHITE, (height - 0.9f) * 10.0f);

		}

		auto drawSolidBars = [=](const Bar& bar)
			{
				DrawingDetails::SolidDrawer a(c);
				a(bar);
			};
		auto drawGhostBars = [=](const Bar& bar)
			{
				DrawingDetails::GhostDrawer a(ColorAlpha(VIS_PURPLE, 0.4f));
				a(bar);
			};

		visBars.emplace_back(Bar{ mainHeight, (int)barWidth, xLeft, xRight, mainY }, drawSolidBars);
		visBars.emplace_back(Bar{ ghostHeight, (int)barWidth, xLeft, xRight, ghostY }, drawGhostBars);
	};
}

void GraphicsThread::Draw()
{
	// clear and reserve memory for visual bars before drawing
	visBars.clear();
	visBars.reserve(BUCKET_COUNT * 2);

	this->prepareVisuals();

	float bass = smoothState.empty() ? 0.0f : std::min(smoothState[0], 1.0f);
	float bassShock = powf(bass, 3.0f);
	float targetZoom = 1.0f + (bassShock * 0.05f);
	mCamera.zoom += (targetZoom - mCamera.zoom) * 10.0f * GetFrameTime();
	mCamera.rotation = sinf((float)GetTime() * 0.5f) * 2.0f;


	BeginTextureMode(target);

	BeginBlendMode(BLEND_ALPHA);
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0,0,0,70 });
	EndBlendMode();

	BeginMode2D(mCamera);

	BeginBlendMode(BLEND_ADDITIVE);
	Color gridColor = ColorAlpha(CyberpunkColors::NEON_PURPLE, 0.15f);
	float time = (float)GetTime();
	for (int i = 0; i < 20;++i)
	{
		float y = (screenHeight / 2) + (i * i * 2) + (fmodf(time * 20.0f, 20.0f));
		if (y < screenHeight) DrawLine(0, y, screenWidth, y, gridColor);
	}
	DrawLine(0, screenHeight / 2, screenWidth, screenHeight / 2, CyberpunkColors::NEON_PINK);
	EndBlendMode();

	BeginBlendMode(BLEND_ADDITIVE);
	for (auto& bar : this->visBars)
	{
		draw(bar);
	}
	EndBlendMode();
	EndMode2D();
	EndTextureMode();

	BeginDrawing();
	ClearBackground(BLACK);

	float shake = smoothState.empty() ? 0.0f : smoothState[0] * 5.0f;

	Rectangle srcRec = { 0,0, (float)target.texture.width, (float)-target.texture.height };
	Rectangle destRec = { 0, 0, (float)screenWidth, (float)screenHeight };
	Vector2 origin = { 0, 0 };

	BeginBlendMode(BLEND_ADDITIVE);
	DrawTexturePro(target.texture, srcRec,
		{ -shake,0,(float)screenWidth, (float)screenHeight },
		origin, 0.0f, CyberpunkColors::NEON_PINK);
	DrawTexturePro(target.texture, srcRec,
		{ shake,0,(float)screenWidth, (float)screenHeight },
		origin, 0.0f, CyberpunkColors::NEON_CYAN);
	DrawTexturePro(target.texture, srcRec, destRec, origin, 0.0f, WHITE);
	EndBlendMode();

	BeginBlendMode(BLEND_MULTIPLIED);
	DrawCircleGradient(screenWidth / 2, screenHeight / 2,
		screenWidth * 1.3f, BLANK, BLACK);
	EndBlendMode();

	BeginBlendMode(BLEND_ALPHA);
	for (int y = 0; y < screenHeight; y += 4)
	{
		DrawRectangle(0, y, screenWidth, 1, Color{ 0,0,0,100 });
	}
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
