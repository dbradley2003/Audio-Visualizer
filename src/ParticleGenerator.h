#ifndef PARTICLE_GENERATOR_H
#define PARTICLE_GENERATOR_H

#include "Particle.h"
#include "constants.h"
#include <vector>

using namespace CyberpunkColors;
using namespace Constants;

class ParticleGenerator {
public:
  ParticleGenerator(const ParticleGenerator &) = delete;
  ParticleGenerator() = default;
  void Init();
  void Update(float rawBass, float rawTreble) const;
  void Draw() const;

private:
  mutable std::vector<Particle> particles;
  Texture2D particleSprite;
  Texture2D whitePixel;
  int width{0};
  int height{0};
};

#endif
