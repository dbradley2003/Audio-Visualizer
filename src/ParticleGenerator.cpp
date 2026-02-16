#include "ParticleGenerator.h"

void ParticleGenerator::Init() {
  this->width = GetScreenWidth();
  this->height = GetScreenHeight();

  const Image img =
      GenImageGradientRadial(64, 64, 0.0f, WHITE, (Color){255, 255, 255, 0});
  this->particleSprite = LoadTextureFromImage(img);
  UnloadImage(img);

  const Image whiteImg = GenImageColor(1, 1, WHITE);
  whitePixel = LoadTextureFromImage(whiteImg);
  UnloadImage(whiteImg);

  particles.clear();
  particles.reserve(PARTICLE_COUNT);

  for (int i{}; i < PARTICLE_COUNT; ++i) {
    float size{0.0f};
    Vector2 velocity;
    float alpha{0.0f};

    if (GetRandomValue(0, 100) < 60) {
      size = static_cast<float>(GetRandomValue(1, 2)) * 0.8f;
      const auto velX = static_cast<float>(GetRandomValue(-20, 20));
      const auto velY = static_cast<float>(GetRandomValue(-20, 20));
      velocity = {velX, velY};
      alpha = 0.25f;
    } else {
      size = static_cast<float>(GetRandomValue(2, 4)) * 0.8f;
      const auto velX = static_cast<float>(GetRandomValue(-100, 100));
      const auto velY = static_cast<float>(GetRandomValue(-100, 100));
      velocity = {velX, velY};
      alpha = 1.0f;
    }

    const auto posX = static_cast<float>(GetRandomValue(0, width));
    const auto posY = static_cast<float>(GetRandomValue(0, height));

    Vector2 position{posX, posY};
    const auto id = static_cast<float>(i);
    Color base = P_NEON_CYAN;
    particles.emplace_back(position, velocity, size, alpha, 0.0f, base, id);
  }
}

void ParticleGenerator::Draw() const {
  const Rectangle source = {0.0f, 0.0f,
                            static_cast<float>(particleSprite.width),
                            static_cast<float>(particleSprite.height)};
  const Rectangle srcPixel = {0.0f, 0.0f, 1.0f, 1.0f};

  BeginBlendMode(BLEND_ADDITIVE);
  for (const auto &p : particles) {
    if (p.alpha() <= 0.01f) {
      continue;
    }
    const float coreSize = p.pulse() * 0.5f;
    const float glowSize = coreSize * 3.0f;
    const float glowRadius = glowSize / 2.0f;

    const auto [x, y] = p.position();
    const Color drawColor = ColorAlpha(p.base(), p.alpha());

    DrawTexturePro(particleSprite, source,
                   (Rectangle){x, y, glowSize, glowRadius},
                   (Vector2){glowRadius, glowRadius}, 0.0f, drawColor);

    DrawTexturePro(whitePixel, srcPixel, (Rectangle){x, y, coreSize, coreSize},
                   (Vector2){coreSize / 2.0f, coreSize / 2.0f}, 0.0f,
                   ColorAlpha(WHITE, p.alpha()));
  }
  EndBlendMode();
}

void ParticleGenerator::Update(const float rawBass,
                               const float rawTreble) const {
  // Constants
  const float dt = GetFrameTime();
  const auto time = static_cast<float>(GetTime());

  constexpr float border = 50.0f;
  const float wLimit = static_cast<float>(this->width) + border;
  const float hLimit = static_cast<float>(this->height) + border;

  const float bass = std::clamp(rawBass, 0.0f, 1.0f);
  const float treble = std::clamp(rawTreble, 0.0f, 1.0f);

  const float bassShock = powf(bass, 3.0f);
  const float globalSpeed = 1.0f + (bassShock * 4.0f);

  for (auto &p : particles) {

    const float noiseX = p.position().x * 0.005f;
    const float noiseY = p.position().y * 0.005f;

    const float flowAngle =
        FastSin(noiseX + time * 0.5f) + FastSin(noiseY + 1.5707f) * PI;

    const Vector2 flowVec = {cosf(flowAngle), sinf(flowAngle)};
    const float moveX = (p.velocity().x * 0.8f) + (flowVec.x * 20.0f * globalSpeed);
    const float moveY = (p.velocity().y * 0.8f) + (flowVec.y * 20.0f * globalSpeed);

    float posX = p.position().x + (moveX * dt * 2.0f);
    float posY = p.position().y + (moveY * dt * 2.0f);

    if (posX < -border) {
      posX = wLimit;
    } else if (posX > wLimit) {
      posX = -border;
    }

    if (posY < -border) {
      posY = hLimit;
    } else if (posY > hLimit) {
      posY = -border;
    }

    const float pulse =
        1.0f + (FastSin(time * 5.0f + p.id()) * 0.2f) + (bassShock * 0.8f);
    Color targetColor = ColorLerp(P_DEEP_VOID, P_NEON_CYAN, bass);
    if (treble < 0.6f)
      targetColor = ColorLerp(targetColor, WHITE, (treble - 0.6f) * 2.5f);

    p.setPosition({posX, posY});
    p.setPulse(p.size() * pulse);
    p.setBase(targetColor);
    p.setAlpha(0.3f + (bassShock * 0.7f));
  }
}
