#pragma once

#include <gint/display.h>
#include <stdbool.h>

#include "inventory.h"
#include "render.h"
#include "entity.h"

#define MAX_FRIENDS 3

#define getTile(x, y) (world.tiles[(y) * WORLD_WIDTH + (x)])

enum SupportTypes {
	SUPPORT_NONE,
	SUPPORT_KEEP,
	SUPPORT_NEED
};

/* I want to keep as much data as I can in here
and not the Tile struct */
typedef struct {
	bopti_image_t* sprite;
	enum PhysicsTypes physics;
	bool render;
	enum SpriteTypes spriteType;
//	Stop player breaking tiles below it
	enum SupportTypes support;
/*	Tiles with spritesheets will treat tiles in here
	as this tile e.g. dirt and stone */
	unsigned char friends[MAX_FRIENDS];
	enum Items item;
	char* name;
} TileData;

extern const TileData tiles[];

typedef struct {
//	Index in tiles array
	unsigned char idx:6;
	unsigned char variant:2;
} Tile;

enum Tiles {
	TILE_NULL = -1,
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT,
	TILE_GRASS,
	TILE_WOOD,
	TILE_TRUNK,
	TILE_ROOT_L,
	TILE_ROOT_R,
	TILE_LEAVES,
	TILE_PLANT,
	TILE_WBENCH_L,
	TILE_WBENCH_R,
	TILE_PLATFORM,
	TILE_CHAIR_L,
	TILE_CHAIR_R,

	TILES_COUNT
};

struct World {
	Tile* tiles;

	void (*placeTile)(int x, int y, Item* item);
	void (*removeTile)(int x, int y);
};
	
extern struct World world;

void generateWorld();
void updateStates(int x, int y);
void regionChange(int x, int y);
unsigned char makeVar();
void breakTree(int x, int y);
unsigned char findState(int x, int y);

void placeTile(int x, int y, Item* item);
void removeTile(int x, int y);