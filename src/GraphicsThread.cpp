#include "GraphicsThread.h"

#include "raylib.h"
#include <cmath>
#include "Bar.h"
#include "Drawable.h"
#include <functional>

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
	visBars(BUCKET_COUNT * 2)
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
	int index = 0;
	int centerY = screenHeight / 2;

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
			c = ColorLerp(VIS_BLUE, VIS_PURPLE, height * 2.0f);
		}
		else
		{
			c = ColorLerp(VIS_PURPLE, WHITE, (height - 0.5f) * 2.0f);
		}
		
		Color ghostColor = ColorAlpha(VIS_YELLOW, 0.4f);
		std::function<void(const Bar&)> func = 
			DrawingDetails::GhostDrawer(ghostColor);
		std::function<void(const Bar&)> func2 = 
			DrawingDetails::SolidDrawer(c);

		visBars[index++] = Drawable{
			Bar{ghostHeight, (int)barWidth, xLeft, xRight, ghostY},func};
		visBars[index++] = Drawable{
			Bar{mainHeight, (int)barWidth, xLeft, xRight, mainY},func2 };
	};
}

void GraphicsThread::Draw()
{
	this->prepareVisuals();
	BeginTextureMode(target);
	BeginBlendMode(BLEND_MULTIPLIED);
	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 5,10,20,30 });
	EndBlendMode();

	BeginBlendMode(BLEND_ADDITIVE);

	for (auto& bar : this->visBars)
	{
		draw(bar);
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
}

void DrawCoolRectangle(float x, float y, float width, float height, Color color)
{
	DrawRectangle(x, y, width, height, color);     //Use passed color
	DrawRectangleLines(x, y, width, height, ColorAlpha(color, 0.3f)); // Gruvbox foreground
	DrawCircle(x + width / 2, y, width / 4, ColorAlpha(color, 0.2f)); // Aqua as an accent
}
