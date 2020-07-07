#pragma once

#include <gint/display.h>
#include <stdbool.h>
#include <gint/std/stdlib.h>

typedef struct Tiles {
	image_t* sprite;
	bool solid:1;
	bool render:1;
} Tile;

extern const Tile tiles[];

enum Tilenames {
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT
};

struct Map {
	const Tile** tiles;
};

void generateMap(struct Map* map);