#pragma once

#include <gint/display.h>
#include <stdbool.h>

#define MAX_FRIENDS 2

/* I want to keep as much data as I can in here
and not the Tile struct */
typedef struct TileDataStruct {
	image_t* sprite;
	bool solid;
	bool render;
	bool hasSpritesheet;
/*	Tiles with spritesheets will treat tiles in here
	as this tile e.g. dirt and stone */
	unsigned char friends[MAX_FRIENDS];
} TileData;

extern const TileData tiles[];

typedef struct TileStruct {
//	Index in tiles array
	unsigned char idx:6;
//	For spritesheet tiles
	unsigned char state:4;
} Tile;

enum Tilenames {
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT,
	TILE_GRASS
};

struct World {
	Tile* tiles;
};

void generateWorld(struct World* world);
void updateStates(struct World* world, int x, int y);