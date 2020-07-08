#pragma once

#include <gint/display.h>
#include <stdbool.h>

typedef struct Tiles {
	image_t* sprite;
	bool solid:1;
	bool render:1;
} Tile;

extern const Tile tiles[];

enum Tilenames {
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT,
	TILE_GRASS
};

struct World {
	unsigned char* tiles;
};

void generateWorld(struct World* world);