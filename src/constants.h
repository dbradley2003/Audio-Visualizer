#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "raylib.h"
#include <array>

constexpr double pi = 3.14159265358979323846;
constexpr int TABLE_SIZE = 2048;

namespace Constants {
static constexpr int FFT_SIZE = 4096;
static constexpr int BIN_COUNT = FFT_SIZE / 2;
static constexpr int HOP_SIZE = 512;
static constexpr int BUCKET_COUNT = 128;
static constexpr float BAR_SPACING = 2.0f;
static constexpr float SMOOTHNESS = 10.0f;
static constexpr float SMEAREDNESS = 3.0f;
static constexpr int PARTICLE_COUNT = 1000;
static constexpr float TREBLE_MULTIPLIER = 3.0f;
} // namespace Constants

namespace CyberpunkColors {
static constexpr Color DARK_GRAY = {20, 40, 50, 255};
static constexpr Color NEON_CYAN = {0, 220, 255, 255};   // TRON Blue
static constexpr Color NEON_PINK = {255, 0, 175, 255};   // Hotline Miami Pink
static constexpr Color NEON_GREEN = {0, 255, 65, 255};   // Matrix Green
static constexpr Color NEON_PURPLE = {180, 0, 255, 255}; // Deep Neon
static constexpr Color DARK_CYAN = {0, 15, 20, 255};
static constexpr Color NEON_ORANGE = {255, 60, 0, 255};
static constexpr Color ICE_BLUE = {180, 255, 255, 255};
static constexpr Color P_DEEP_VOID = {20, 0, 40, 255};
static constexpr Color P_NEON_CYAN = {0, 240, 255, 255};
static constexpr Color P_HYPER_HOT = {255, 255, 200, 255};
const Color GRID_COLOR = ColorAlpha(Color{0, 100, 120, 255}, 0.3f);

static constexpr Color GHOST_BASE = {0, 220, 255, 255};
static constexpr Color GHOST_DROP = {255, 60, 0, 255};
static constexpr Color DROP_TOP = {40, 0, 0, 255};
static constexpr Color DROP_BOTTOM = {255, 60, 0, 255};

} // namespace CyberpunkColors

constexpr double const_sin(double x) {
  while (x > pi)
    x -= 2 * pi;
  while (x < -pi)
    x += 2 * pi;

  double res = x;
  double term = x;
  double x2 = x * x;

  term *= -x2 / (2 * 3);
  res += term;
  term *= -x2 / (4 * 5);
  res += term;
  term *= -x2 / (6 * 7);
  res += term;
  term *= -x2 / (8 * 9);
  res += term;
  term *= -x2 / (10 * 11);
  res += term;
  return res;
}

constexpr std::array<float, TABLE_SIZE> GenerateSineTable() {
  std::array<float, TABLE_SIZE> table{};
  for (int i{}; i < TABLE_SIZE; ++i) {
    const double angle = (static_cast<double>(i) / TABLE_SIZE) * 2.0 * pi;
    table[i] = static_cast<float>(const_sin(angle));
  }
  return table;
}

static constexpr auto SINE_TABLE = GenerateSineTable();

inline float FastSin(float angle) {
  float norm = angle / (2.0f * (float)pi);
  norm = norm - (long)norm;
  if (norm < 0)
    norm += 1.0f;

  int index = static_cast<int>(norm * TABLE_SIZE);

  index = index >= TABLE_SIZE ? TABLE_SIZE - 1 : index;

  return SINE_TABLE[index];
}

#endif