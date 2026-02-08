#include "GraphicsThread.h"

#include "Bar.h"
#include "raylib.h"
#include "raymath.h"
#include <sys/stat.h>

#pragma warning(disable : 4244)

using namespace std;
using namespace CyberpunkColors;

namespace {
class GhostDrawer {
public:
  explicit GhostDrawer(const Color baseColor)
      : mColTop(ColorAlpha(baseColor, 0.3f)),
        mColBot(ColorAlpha(baseColor, 0.0f)),
        mShadowCol(ColorAlpha(BLACK, 0.5f)) {}

  void operator()(const Bar &bar) const {
    if (bar.height() <= 2) {
      return;
    }

    const int w = bar.width();
    const int h = bar.height();

    DrawRectangle(bar.xRight(), bar.y(), w, h, mShadowCol);
    DrawRectangle(bar.xLeft(), bar.y(), w, h, mShadowCol);
    DrawRectangleGradientV(bar.xRight(), bar.y(), w, bar.height(), mColTop,
                           mColBot);
    DrawRectangleGradientV(bar.xLeft(), bar.y(), w, h, mColTop, mColBot);
  };

private:
  Color mColTop;
  Color mColBot;
  Color mShadowCol;
};

class SolidDrawer {
public:
  explicit SolidDrawer(const Color topColor, const Color bottomColor)
      : mTopColor(topColor), mBottomColor(bottomColor),
        mGlowColor(ColorAlpha(topColor, 0.6f)),
        mCapColor(ColorAlpha(WHITE, 0.95f)) {}

  void operator()(const Bar &bar) const {
    if (bar.height() < 1)
      return;

    const int w = bar.width();
    const int h = bar.height();

    DrawRectangleGradientV(bar.xRight(), bar.y(), w, h, mTopColor,
                           mBottomColor);
    DrawRectangleGradientV(bar.xLeft(), bar.y(), w, h, mTopColor, mBottomColor);

    DrawRectangle(bar.xLeft(), bar.y() - 2, w, 4, mGlowColor);
    DrawRectangle(bar.xRight(), bar.y() - 2, w, 4, mGlowColor);

    DrawRectangle(bar.xLeft(), bar.y(), w, 2, mCapColor);
    DrawRectangle(bar.xRight(), bar.y(), w, 2, mCapColor);
  };

private:
  Color mTopColor;
  Color mBottomColor;
  Color mGlowColor;
  Color mCapColor;
};
} // namespace

GraphicsThread::GraphicsThread(const int screenHeight, const int screenWidth,
                               TripleBuffer<std::vector<float>> &sharedBuffer)
    : screenHeight(screenHeight), screenWidth(screenWidth),
      share_ag(sharedBuffer), smoothState(BUCKET_COUNT, 0.0f),
      smearedState(BUCKET_COUNT, 0.0f), colorLUT(), target(), mCamera()

{
  visBars.reserve(BUCKET_COUNT * 2);
}

void GraphicsThread::PrecomputeGradient() {
  for (int i = 0; i < 256; ++i) {
    const float t = i / 255.0f;
    if (t < 0.8f) {
      colorLUT[i] = ColorLerp(BLACK, NEON_CYAN, t * 1.25f);
    } else {
      const float rangeT = (t - 0.8f) * 5.0f;
      Color base = ColorLerp(NEON_CYAN, ICE_BLUE, rangeT);

      if (t > 0.9f) {
        const float whiteT = (t - 0.9f) * 10.0f;
        base = ColorLerp(base, WHITE, whiteT);
      }
      colorLUT[i] = base;
    }
  }
}

void GraphicsThread::Initialize() {

  SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "Audio Visualizer");

  const auto scaledH = static_cast<float>(screenHeight);
  const auto scaledW = static_cast<float>(screenWidth);

  target = LoadRenderTexture(screenWidth, screenHeight);
  SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

  readBuffer = share_ag.consumerReadBuffer();
  particleGenerator.Init();

  const float halfH = scaledH * 0.5f;
  const float halfW = scaledW * 0.5f;
  const float availableW = halfW - (BAR_SPACING * BUCKET_COUNT);
  halfWidth = halfW;
  barWidth = std::max(std::floor(availableW / BUCKET_COUNT), 1.0f);

  mCamera = {{halfW, halfH}, {halfW, halfH}, 0.0f, 1.0f};
  this->PrecomputeGradient(); // Initialize Color Gradient LUT
}

bool GraphicsThread::Swap() { return share_ag.swapConsumer(this->readBuffer); }

void GraphicsThread::Update() {

  // Recycle used buffer and get fresh audio data
  this->Swap();

  if (!this->readBuffer || this->readBuffer->empty()) {
    return;
  }
  this->fftProcess();
}

void GraphicsThread::fftProcess() {
  const auto NUM_BINS = static_cast<float>(this->readBuffer->size()) * 0.5f;

  for (int bar = 0; bar < BUCKET_COUNT; ++bar) {
    const auto barFloat = static_cast<float>(bar);

    const float tStart = barFloat / BUCKET_COUNT;
    const float tEnd = (barFloat + 1.0f) / BUCKET_COUNT;

    int fStart = static_cast<int>(powf(tStart, 2.5f) * NUM_BINS);
    int fEnd = static_cast<int>(powf(tEnd, 2.5f) * NUM_BINS);

    if (fStart < 2)
      fStart = 2;
    if (fEnd <= fStart)
      fEnd = fStart + 1;
    if (fEnd > static_cast<int>(NUM_BINS))
      fEnd = static_cast<int>(NUM_BINS);

    float sum = 0.0f;
    int count = 0;

    for (int q = fStart; q < fEnd; ++q) {
      sum += (*readBuffer)[q];
      count++;
    }
    const float dt = GetFrameTime();
    const float val = (count > 0) ? sum / static_cast<float>(count) : 0.0f;

    smoothState[bar] += (val - smoothState[bar]) * SMOOTHNESS * dt;
    smearedState[bar] +=
        (smoothState[bar] - smearedState[bar]) * SMEAREDNESS * dt;
  }
}

GraphicsThread::~GraphicsThread() { UnloadRenderTexture(target); }

void GraphicsThread::prepareVisuals() {
  const int centerY = screenHeight / 2;
  const auto heightScale = static_cast<float>(this->screenHeight);

  const int size = static_cast<int>(smoothState.size());
  const int maxBin = std::min(size, 20);
  const float bass = smoothState.empty() ? 0.0f : smoothState[0];

  const float bassShock = bass * bass * bass;

  float treble = 0.0f;
  if (!smoothState.empty()) {
    int count = 0;
    for (int i = 5; i < maxBin; ++i) {
      treble += smoothState[i];
      ++count;
    }
    if (count > 0) {
      treble = (treble / static_cast<float>(count)) * TREBLE_MULTIPLIER;
    }
  }

  particleGenerator.update(bass, treble);
  const Color ghostColor = ColorLerp(GHOST_BASE, GHOST_DROP, bassShock);

  visBars.clear();
  visBars.reserve(BUCKET_COUNT * 2);

  Color targetDropTop = DROP_TOP;
  float currentOffset = 0.0f;
  const float stride = barWidth + BAR_SPACING;

  const GhostDrawer ghostDrawer(ColorAlpha(ghostColor, 0.25f));

  if (bassShock > 0.8f) {
    targetDropTop =
        ColorLerp(DROP_TOP, {255, 255, 200, 255}, (bassShock - 0.8f) * 5.0f);
  }

  for (int i = 0; i < BUCKET_COUNT; ++i) {
    const float smear = smearedState[i];
    const float smooth = smoothState[i];

    if (smooth <= 0.001f && smear <= 0.001f) {
      currentOffset += stride;
      continue;
    }

    const int colorIndex = static_cast<int>(std::min(smooth, 1.0f) * 255.0f);
    const Color normalTop = colorLUT[colorIndex];
    constexpr Color normalBottom = DARK_CYAN;

    const int xRight = static_cast<int>(halfWidth + currentOffset);
    const int xLeft = static_cast<int>(halfWidth - currentOffset - barWidth);

    const int ghostH = static_cast<int>(smear * heightScale);
    const int mainH = static_cast<int>(smooth * heightScale);

    const int ghostY = centerY - (ghostH / 2);
    const int mainY = centerY - (mainH / 2);

    const Color finalTop = ColorLerp(normalTop, targetDropTop, bassShock);
    const Color finalBot = ColorLerp(normalBottom, DROP_BOTTOM, bassShock);

    auto solidBars = [=](const Bar &bar) {
      const SolidDrawer sd(finalTop, finalBot);
      sd(bar);
    };

    auto reflectionBars = [=](const Bar &bar) {
      DrawRectangleGradientV(bar.xLeft(), bar.y() + bar.height() + 5,
                             bar.width(), static_cast<int>(bar.height() * 0.4f),
                             ColorAlpha(finalBot, 0.15f),
                             ColorAlpha(BLACK, 0.0f));
    };

    auto ghostBars = [ghostDrawer](const Bar &bar) { ghostDrawer(bar); };

    if (mainH > 0) {
      visBars.emplace_back(
          Bar{mainH, static_cast<int>(barWidth), xLeft, xRight, mainY},
          solidBars);
    }

    if (ghostH > 0) {
      visBars.emplace_back(
          Bar{ghostH, static_cast<int>(barWidth), xLeft, xRight, ghostY},
          ghostBars);
    }

    visBars.emplace_back(
        Bar{mainH, static_cast<int>(barWidth), xLeft, xRight, ghostY},
        reflectionBars);

    currentOffset += stride;
  }
}

void GraphicsThread::DrawGridLines() const {
  const auto SCALED_HEIGHT = static_cast<float>(this->screenHeight);
  const auto SCREEN_WIDTH = static_cast<float>(this->screenWidth);
  const auto SCREEN_WIDTH_EXT = SCREEN_WIDTH + 200.0f;
  const auto time = static_cast<float>(GetTime());

  const float HORIZON_Y = SCALED_HEIGHT / 2.0f;
  const float CENTER_X = SCREEN_WIDTH / 2.0f;
  const float movement = fmodf(time * 100.0f, 100.0f);

  Vector2 horizontalStart = {-200.0f, 0.0f};
  Vector2 horizontalEnd = {SCREEN_WIDTH_EXT, 0.0f};

  BeginBlendMode(BLEND_ADDITIVE);

  // Drawing Horizontal Lines
  for (int i = 0; i < 25; ++i) {
    const auto iFloat = static_cast<float>(i);
    const float dist = iFloat * iFloat * 2.5f;

    if (dist + HORIZON_Y > SCALED_HEIGHT) {
      break;
    }

    const float y = HORIZON_Y + dist + (movement * iFloat * 0.05f);

    horizontalStart.y = y;
    horizontalEnd.y = y;

    const float fade = std::min(1.0f, iFloat * 2.0f);
    const float alpha = fade * 0.3f;
    DrawLineV(horizontalStart, horizontalEnd, ColorAlpha(GRID_COLOR, alpha));
  }

  const Vector2 verticalStart = {CENTER_X, HORIZON_Y};
  Vector2 verticalEnd = {0.0f, SCALED_HEIGHT};

  // Drawing Vertical Lines
  for (int j = -10; j <= 10; ++j) {
    if (j == 0) {
      continue;
    }

    const auto jFloat = static_cast<float>(j);
    const float x = CENTER_X + (jFloat * SCREEN_WIDTH * 0.15f);
    verticalEnd.x = x;
    DrawLineV(verticalStart, verticalEnd, GRID_COLOR);
  }

  DrawLineV({0, HORIZON_Y}, {SCREEN_WIDTH, HORIZON_Y}, NEON_CYAN);
  DrawLineV({0, HORIZON_Y + 1.0f}, {SCREEN_WIDTH, HORIZON_Y + 1.0f},
            ColorAlpha(NEON_CYAN, 0.5f));
  EndBlendMode();
}

void GraphicsThread::ScreenShake() {
  const float dt = GetFrameTime();

  float bass = 0.0f;
  const int bassCount = std::min(static_cast<int>(smoothState.size()), 3);
  for (int i = 0; i < bassCount; ++i) {
    if (bassCount > 0) {
      bass /= static_cast<float>(bassCount);
    }
  }

  constexpr float BASS_THRESHOLD = 0.65f;
  if (bass > BASS_THRESHOLD) {
    const float impact = (bass - BASS_THRESHOLD) * 1.5f;
    mScreenTrauma += impact;
    if (mScreenTrauma > 1.0f) {
      mScreenTrauma = 1.0f;
    }
  }
  mScreenTrauma -= 1.2f * dt;
  if (mScreenTrauma < 0.0f) {
    mScreenTrauma = 0.0f;
  }

  const float shake = mScreenTrauma * mScreenTrauma;
  const float rX = static_cast<float>(GetRandomValue(-100, 100)) / 100.0f;
  const float rY = static_cast<float>(GetRandomValue(-100, 100)) / 100.0f;
  const float rRot = static_cast<float>(GetRandomValue(-100, 100)) / 100.0f;

  constexpr float MAX_OFFSET = 30.0f;
  constexpr float MAX_ANGLE = 2.0f;

  const float offsetX = MAX_OFFSET * shake * rX;
  const float offsetY = MAX_OFFSET * shake * rY;
  const float angle = MAX_ANGLE * shake * rRot;

  const float scale = 1.0f + (0.05f * shake);

  const Rectangle src = {0, 0, static_cast<float>(target.texture.width),
                         -static_cast<float>(target.texture.height)};
  const Vector2 center = {static_cast<float>(screenWidth) * 0.5f,
                          static_cast<float>(screenHeight) * 0.5f};

  const Rectangle dest = {center.x + offsetX, center.y + offsetY,
                          static_cast<float>(screenWidth) * scale,
                          static_cast<float>(screenHeight) * scale};

  const Vector2 origin = {dest.width * 0.5f, dest.height * 0.5f};

  BeginBlendMode(BLEND_ALPHA);
  DrawTexturePro(target.texture, src, dest, origin, angle, WHITE);
  EndBlendMode();
}

void GraphicsThread::DrawVisualBars() const {
  BeginBlendMode(BLEND_ADDITIVE);
  for (auto &visual : this->visBars) {
    draw(visual);
  }
  EndBlendMode();
}

void GraphicsThread::Draw() {
  this->prepareVisuals();

  const float dt = GetFrameTime();
  const auto time = static_cast<float>(GetTime());

  float rawBass = smoothState.empty() ? 0.0f : smoothState[0];
  if (std::isnan(rawBass)) {
    rawBass = 0.0f;
  }

  const float bass = std::clamp(rawBass, 0.0f, 1.0f);
  if (bass > 0.6f) {
    mBeatEnergy += (bass - 0.6f) * 5.0f * dt;
  }

  mBeatEnergy -= 1.5f * dt;
  mBeatEnergy = std::clamp(mBeatEnergy, 0.0f, 1.0f);

  const float targetZoom = 1.0f + (mBeatEnergy * 0.15f);
  mCamera.zoom = Lerp(mCamera.zoom, targetZoom, 5.0f * dt);

  const float sway = sinf(time * 0.5f) * 0.02f;
  mCamera.rotation = (sway * RAD2DEG) + (mBeatEnergy * 2.0f);

  BeginTextureMode(target);

  BeginBlendMode(BLEND_ALPHA);
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 60});
  EndBlendMode();

  BeginMode2D(mCamera);

  if (mBeatEnergy > 0.1f) {
    BeginBlendMode(BLEND_ADDITIVE);
    const Color glowCol = ColorAlpha(NEON_ORANGE, mBeatEnergy * 0.4f);
    DrawCircleGradient(static_cast<int>(halfWidth),
                       static_cast<int>(screenHeight / 2), halfWidth * 1.5f,
                       glowCol, BLANK);
    EndBlendMode();
  }

  // Drawing Main Drawables (Bars, Particles, GridLines)
  DrawGridLines();
  particleGenerator.Draw();
  DrawVisualBars();

  EndMode2D();
  EndTextureMode();

  this->ScreenShake();
}
