#pragma once

#include <gint/display.h>
#include <stdbool.h>

#include "inventory.h"

#define MAX_FRIENDS 2

#define getTile(x, y) (world.tiles[(y) * WORLD_WIDTH + (x)])

/* I want to keep as much data as I can in here
and not the Tile struct */
typedef struct {
	bopti_image_t* sprite;
	bool solid;
	bool render;
	bool hasSpritesheet;
/*	Tiles with spritesheets will treat tiles in here
	as this tile e.g. dirt and stone */
	unsigned char friends[MAX_FRIENDS];
	enum Items item;
} TileData;

extern const TileData tiles[];

typedef struct {
//	Index in tiles array
	unsigned char idx:6;
//	For spritesheet tiles
	unsigned char state:4;
} Tile;

enum Tilenames {
	TILE_NULL = -1,
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT,
	TILE_GRASS
};

struct World {
	Tile* tiles;
};
	
extern struct World world;

void generateWorld();
void updateStates(int x, int y);
void regionChange(int x, int y);