#ifndef PARTICLE_GENERATOR_H
#define PARTICLE_GENERATOR_H

#include "raylib.h"
#include <vector>

namespace CyberpunkColors {
	const Color DARK_GRAY = { 20,40,50,255 };
	const Color NEON_CYAN = { 0,220,255,255 };  // TRON Blue
	const Color NEON_PINK = { 255, 0, 175, 255 };   // Hotline Miami Pink
	const Color NEON_GREEN = { 0, 255, 65, 255 };    // Matrix Green
	const Color NEON_PURPLE = { 180, 0, 255, 255 };   // Deep Neon
	const Color DARK_CYAN = { 0, 15, 20, 255 };
	const Color NEON_ORANGE = { 255, 60, 0, 255 };
}

struct ParticleGenerator
{
	struct Particle
	{
		Vector2 pos;
		Vector2 velocity;
		float size;
		float alphaOffset;
		float pulseSize;
		Color baseColor;
	};

	mutable std::vector<Particle> particles;

	ParticleGenerator(const ParticleGenerator&) = delete;
	ParticleGenerator()
		:h_(0),
		w_(0)
	{ }

	void Init(int count, int w, int h)
	{
		particles.resize(count);
		for (auto& p : particles)
		{
			p.pos = { (float)GetRandomValue(0,w),(float)GetRandomValue(0,h) };
			p.velocity.x = (float)GetRandomValue(-20, 20);
			p.velocity.y = (float)GetRandomValue(-20, 20);
			p.size = (float)GetRandomValue(2, 4);


			if (GetRandomValue(0, 1) == 0) p.baseColor = CyberpunkColors::NEON_CYAN;
			else						   p.baseColor = WHITE;


			p.alphaOffset = (float)GetRandomValue(0, 100) / 10.0f;
		}
		this->h_ = h;
		this->w_ = w;
	}

	void update(float rawBass, float rawTreble)
	{
		float bass = std::min(rawBass, 1.0f);
		float treble = std::min(rawTreble, 1.0f);

		float bassShock = powf(bass, 3.0f);
		float trebleShock = powf(treble, 2.0f);

		float dt = GetFrameTime();

		float speedMult = 1.0f + (bassShock * 20.0f);

		Color dropColor = { 255,200,0,255 };

		for (auto& p : particles)
		{
			float moveX = p.velocity.x * speedMult;
			float moveY = p.velocity.y * speedMult;

			float maxSpeed = 350.0f;

			if (moveX > maxSpeed) moveX = maxSpeed;
			if (moveX < -maxSpeed) moveX = -maxSpeed;
			if (moveY > maxSpeed) moveY = maxSpeed;
			if (moveY < -maxSpeed) moveY = -maxSpeed;

			p.pos.x += moveX * dt;
			p.pos.y += moveY * dt;

			if (p.pos.x < -20) p.pos.x = (float)w_ + 20;
			if (p.pos.x > w_) p.pos.x = -20;

			if (p.pos.y < -20) p.pos.y = (float)h_ + 20;
			if (p.pos.y > h_ + 20) p.pos.y = -20;


			float pulseSize = p.size * (1.0f + (bassShock * 3.0f));
			float alpha = 0.3f + (trebleShock * 0.7f);

			p.alphaOffset = alpha;
			p.pulseSize = pulseSize;
			p.baseColor = ColorLerp(p.baseColor, dropColor, bassShock);
		}
	}

	void operator()() const
	{
		for (auto& p : particles) {
			Color finalCol = ColorAlpha(p.baseColor, p.alphaOffset);
			float coreSize = p.pulseSize * 0.5f;
			Vector2 corePos = { p.pos.x + coreSize * 0.5f,p.pos.y + coreSize * 0.5f };
			DrawRectangleV(corePos, { coreSize, coreSize }, ColorAlpha(WHITE, p.alphaOffset));

			//Color c = ColorAlpha(p.baseColor, p.alphaOffset);
			//DrawRectangleRec({ p.pos.x - p.pulseSize / 2, p.pos.y - p.pulseSize / 2, p.pulseSize, p.pulseSize }, c);
		}
	}

	int w_;
	int h_;
};

#endif
