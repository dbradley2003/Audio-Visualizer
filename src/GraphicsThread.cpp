#include "GraphicsThread.h"

#include "Bar.h"
#include "raylib.h"
#include "raymath.h"
#include <sys/stat.h>

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
        mCapColor(ColorAlpha(WHITE, 0.9f)) {}

  void operator()(const Bar &bar) const {
    if (bar.height() < 1)
      return;

    const int w = bar.width();
    const int h = bar.height();

    DrawRectangleGradientV(bar.xRight(), bar.y(), w, h, mTopColor,
                           mBottomColor);
    DrawRectangleGradientV(bar.xLeft(), bar.y(), w, h, mTopColor, mBottomColor);

    if (w > 3) {
      const Color core = ColorAlpha(WHITE, 0.3f);
      DrawRectangle(bar.xRight() + w/2 -1, bar.y(), 2, h, core);
      DrawRectangle(bar.xLeft() + w/2 -1, bar.y(), 2, h, core);
    }

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
      smearedState(BUCKET_COUNT, 0.0f), colorLUT(), target() {}

void GraphicsThread::PrecomputeGradient() {
  for (int i = 0; i < 256; ++i) {
    const float t = i / 255.0f;
    if (t < 0.8f) {
      colorLUT[i] = ColorLerp(NEON_PURPLE, NEON_CYAN, t);
    } else {
      const float rangeT = (t - 0.8f) * 5.0f;
      Color base = ColorLerp(NEON_CYAN, NEON_PINK, rangeT);

      // if (t > 0.9f) {
      //   const float whiteT = (t - 0.9f) * 10.0f;
      //   base = ColorLerp(base, WHITE, whiteT);
      // }
      colorLUT[i] = base;
    }
  }
}

void GraphicsThread::Initialize() {

  SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "Audio Visualizer");
  const auto scaledW = static_cast<float>(screenWidth);

  target = LoadRenderTexture(screenWidth, screenHeight);
  SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

  readBuffer = share_ag.consumerReadBuffer();
  particleGenerator.Init();

  const float halfW = scaledW * 0.5f;
  const float availableW = halfW - (BAR_SPACING * BUCKET_COUNT);
  halfWidth = halfW;
  barWidth = std::max(std::floor(availableW / BUCKET_COUNT), 1.0f);

  // initialize color gradient look up table
  this->PrecomputeGradient();
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
  constexpr float maxFreqCrop = 0.8f;
  const auto rawBins = static_cast<float>(this->readBuffer->size());
  const float numBins = rawBins * maxFreqCrop;
  const float dt = GetFrameTime();

  constexpr float GRAVITY = 1.2f;

  for (int visualBar = 0; visualBar < BUCKET_COUNT; ++visualBar) {
    const auto fBar = static_cast<float>(visualBar);
    const float tStart = fBar / BUCKET_COUNT;
    const float tEnd = (fBar + 1.0f) / BUCKET_COUNT;

    int fStart = static_cast<int>(powf(tStart, 2.5f) * numBins);
    int fEnd = static_cast<int>(powf(tEnd, 2.5f) * numBins);

    if (fStart < 2)
      fStart = 2;
    if (fEnd <= fStart)
      fEnd = fStart + 1;
    if (fEnd > static_cast<int>(numBins))
      fEnd = static_cast<int>(numBins);

    float sum = 0.0f;
    int count = 0;

    for (int q = fStart; q < fEnd; ++q) {
      sum += (*readBuffer)[q];
      count++;
    }

    const float val = (count > 0) ? sum / static_cast<float>(count) : 0.0f;

    smoothState[visualBar] += (val - smoothState[visualBar]) * SMOOTHNESS * dt;

    if (smoothState[visualBar] > smearedState[visualBar]) {
      smearedState[visualBar] = smoothState[visualBar];
    } else {
      smearedState[visualBar] -= GRAVITY * dt;
      if (smearedState[visualBar] < 0.0f) smearedState[visualBar] = 0.0f;
    }

    // smearedState[visualBar] +=
    //  (smoothState[visualBar] - smearedState[visualBar]) * SMEAREDNESS * dt;
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

  particleGenerator.Update(bass, treble);
  const Color ghostColor = ColorLerp(GHOST_BASE, GHOST_DROP, bassShock);

  visBars.clear();
  visBars.reserve(BUCKET_COUNT * 3);

  Color targetDropTop = DROP_TOP;
  float currentOffset = 0.0f;
  const float stride = barWidth + BAR_SPACING;

  const GhostDrawer ghostDrawer(ColorAlpha(ghostColor, 0.25f));

  if (bassShock > 0.8f) {
    targetDropTop =
        ColorLerp(DROP_TOP, {255, 215, 0, 255}, (bassShock - 0.8f) * 2.0f);
  }

  for (int i = 0; i < BUCKET_COUNT; ++i) {
    const float smear = smearedState[i];
    const float smooth = smoothState[i];

    if (smooth <= 0.001f && smear <= 0.001f) {
      currentOffset += stride;
      continue;
    }

    const int colorIndex = static_cast<int>(std::min(smooth * 155.0f, 255.0f));
    const Color normalTop = colorLUT[colorIndex];
    const Color normalBottom = ColorAlpha(normalTop, 0.2f);

    const int xRight = static_cast<int>(halfWidth + currentOffset);
    const int xLeft = static_cast<int>(halfWidth - currentOffset - barWidth);

    const int ghostH = static_cast<int>(smear * heightScale);
    const int mainH = static_cast<int>(smooth * heightScale);

    const int ghostY = centerY - (ghostH / 2);
    const int mainY = centerY - (mainH / 2);

    float intensity = std::min(bassShock, 0.6f);
    const Color finalTop = ColorLerp(normalTop, targetDropTop, intensity);
    const Color finalBot =
        ColorLerp(normalBottom, DROP_BOTTOM, bassShock * 0.3f);

    auto solidBars = [=](const Bar &bar) {
      const SolidDrawer sd(finalTop, finalBot);
      sd(bar);
    };

    auto reflectionBars = [=](const Bar &bar) {
      DrawRectangleGradientV(
          bar.xLeft(), bar.y() + bar.height() + 5, bar.width(),
          static_cast<int>(static_cast<float>(bar.height()) * 0.4f),
          ColorAlpha(finalBot, 0.15f), ColorAlpha(BLACK, 0.0f));
    };

    auto ghostBars = [ghostDrawer](const Bar &bar) { ghostDrawer(bar); };

    if (ghostH > 0) {
      visBars.emplace_back(
          Bar{ghostH, static_cast<int>(barWidth), xLeft, xRight, ghostY},
          ghostBars);
    }

    if (mainH > 0) {
      visBars.emplace_back(
          Bar{mainH, static_cast<int>(barWidth), xLeft, xRight, mainY},
          reflectionBars);

      visBars.emplace_back(
          Bar{mainH, static_cast<int>(barWidth), xLeft, xRight, mainY},
          solidBars);
    }

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

  if (bass > 0.6f) {
    const float impact = (bass - 0.6f) * 1.5f;
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
  BeginTextureMode(target);
  BeginBlendMode(BLEND_ALPHA);
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 60});
  EndBlendMode();
  // Drawing Main Drawables (Bars, Particles, GridLines)

  DrawGridLines();
  particleGenerator.Draw();
  DrawVisualBars();
  EndTextureMode();
  this->ScreenShake();
}
