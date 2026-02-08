#ifndef PARTICLE_GENERATOR_H
#define PARTICLE_GENERATOR_H

#include "constants.h"
#include "Particle.h"

using namespace CyberpunkColors;
using namespace Constants;

struct ParticleGenerator
{
	mutable std::vector<Particle> particles;
	ParticleGenerator(const ParticleGenerator&) = delete;
	ParticleGenerator() = default;

	void Init()
	{
		width = GetScreenWidth();
		height = GetScreenHeight();

		particles.clear();
		particles.reserve(PARTICLE_COUNT);

		
		
		for (int i{}; i < PARTICLE_COUNT; ++i)
		{
			Particle p{};
			bool isBackground = GetRandomValue(0, 100) < 80;
			
			if (isBackground)
			{
				p.size = (float)GetRandomValue(1, 2) * 0.85f;
				p.velocity.x = (float)GetRandomValue(-20, 20);
				p.velocity.y = (float)GetRandomValue(-20, 20);
				p.alphaOffset = 0.25f;
			}
			else
			{
				p.size = (float)GetRandomValue(1,2) * 0.5;
				p.velocity.x = (float)GetRandomValue(-100, 100);
				p.velocity.y = (float)GetRandomValue(-100, 100);
				p.alphaOffset = 1.0f;
			}

			p.pos = { (float)GetRandomValue(0,width),(float)GetRandomValue(0,height) };
			p.id = (float)i;
			p.baseColor = P_NEON_CYAN;

			particles.push_back(p);
		}
	}

	void update(float rawBass, float rawTreble)
	{
		// Constants
		const float dt = GetFrameTime();
		const float maxSpeed = 200.0f;
		const float border = 50.0f;
		const float widthLimit = (float)width + border;
		const float heightLimit = (float)height + border;

		float bass = std::clamp(rawBass,0.0f, 1.0f);
		float treble = std::clamp(rawTreble,0.0f, 1.0f);
		
		float bassShock = bass * bass * bass;
		float trebleShock = treble * treble;

		float globalSpeed = 1.0f + (bassShock * 5.0f);
		float globalPulse = 1.0f + (bassShock * 1.5f);

		for (auto& p : particles)
		{
			float depthFactor = (p.size / 4.0f);
			float depthSpeed = globalSpeed * (0.5f + depthFactor);

			float driftX = FastSin(p.id * 1.7f + (float)GetTime()) * 20.0f;
			float driftY = FastSin(p.id * 2.3f + (float)GetTime() + 2.0f) * 40.0f;

			float rawMoveX = (p.velocity.x + driftX) * depthSpeed;
			float rawMoveY = (p.velocity.y + driftY) * depthSpeed;

			p.pos.x += std::clamp(rawMoveX, -maxSpeed, maxSpeed) * dt;
			p.pos.y += std::clamp(rawMoveY, -maxSpeed, maxSpeed) * dt;

			if (p.pos.x < -border) p.pos.x = widthLimit;
			else if (p.pos.x > widthLimit) p.pos.x = -border;

			if (p.pos.y < -border) p.pos.y = heightLimit;
			else if (p.pos.y > heightLimit) p.pos.y = -border;

			p.pulseSize = p.size * globalPulse;

			float wave = FastSin((float)GetTime() * 3.0f + p.id * 0.5f);
			float rawEnergy = bassShock + (wave * 0.15f);
			const float energy = std::clamp(rawEnergy, 0.0f, 1.0f);

			if (energy < 0.5f) p.baseColor = ColorLerp(P_DEEP_VOID, P_NEON_CYAN, energy * 2.0f);
			else p.baseColor = ColorLerp(P_NEON_CYAN, P_HYPER_HOT, (energy - 0.5f) * 2.0f);

			p.alphaOffset = 0.2f + (trebleShock * 0.8f);
		}
	}

	void Draw() const
	{
		BeginBlendMode(BLEND_ADDITIVE);
		for (const auto& p : particles) {
			if (p.alphaOffset <= 0.01f) continue;

			float coreSize = p.pulseSize * 0.5f;

			Vector2 corePos = {
				p.pos.x + p.pulseSize * 0.35f,
				p.pos.y + p.pulseSize * 0.35f
			};

			//DrawCircleGradient((int)corePos.x, (int)corePos.y, coreSize * 2.0f, ColorAlpha(p.baseColor, p.alphaOffset),
			//	ColorAlpha(BLACK, 0.0f));

			DrawRectangleV(corePos, { coreSize, coreSize }, ColorAlpha(p.baseColor, p.alphaOffset));
		}
		EndBlendMode();
	}

	int width{ 0 };
	int height{ 0 };
};

#endif
