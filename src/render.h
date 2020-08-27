#pragma once

enum SpriteTypes {
	TYPE_TILE,
	TYPE_TILE_VAR,
	TYPE_SHEET,
	TYPE_SHEET_VAR
};

typedef struct {
	int x;
	int y;
	float xVel;
	float yVel;
} Particle;

struct ParticleExplosion {
	int numParticles;
	Particle *particles;
	int deltaTicks;
};

typedef struct {
	unsigned char pixel0 : 4;
	unsigned char pixel1 : 4;
} Pair;

void render();
void takeVRAMCapture();
void renderItem();

void createExplosion(struct ParticleExplosion *explosion, int x, int y);
void renderAndUpdateExplosion(struct ParticleExplosion *explosion, int offsetX, int offsetY);