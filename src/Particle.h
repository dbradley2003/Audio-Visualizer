#ifndef PARTICLE_H
#define PARTICLE_H

#include "raylib.h"

struct Particle
{
	Vector2 pos;
	Vector2 velocity;
	float size;
	float alphaOffset;
	float pulseSize;
	Color baseColor;
	float id;
};

#endif
