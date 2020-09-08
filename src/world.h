#pragma once

/*
----- WORLD -----

The world generation system and API functions.
*/

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
	bopti_image_t *sprite;
	enum PhysicsTypes physics;
	bool render;
	enum SpriteTypes spriteType;
//	Stop player breaking tiles below it
	enum SupportTypes support;
/*	Tiles with spritesheets will treat tiles in here
	as this tile e.g. dirt and stone */
	unsigned char friends[MAX_FRIENDS];
	enum Items item;
	char *name;
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
	TILE_CHAIR,

	TILES_COUNT
};

struct World {
	Tile *tiles;
	Entity *entities;
	struct ParticleExplosion explosion;

	void (*placeTile)(int x, int y, Item *item);
	void (*removeTile)(int x, int y);
	int (*spawnEntity)(enum Entities entity, int x, int y);
	bool (*removeEntity)(int idx);
	bool (*checkFreeEntitySlot)();
};
	
extern struct World world;

/* generateWorld
Generates a world.
*/
void generateWorld();

/* regionChange
Sets the status of the region the given coordinates are in. Used to keep track
of which world regions are to be saved to file.

x, y: Coordinates of a tile within the world.
*/
void regionChange(int x, int y);

/* makeVar
Returns a random number between 0 and 2 inclusive, used for tile variations.

Returns the random number.
*/
unsigned char makeVar();

/* breakTree
Breaks a tree, starting from the given trunk coordinate.

x, y: Coordinates of a TILE_TRUNK tile.
*/
void breakTree(int x, int y);

/* findState
Returns the appropriate state for the given tile, given its surroundings.

x, y: Coordinates of a tile within the world.

Returns the state of the tile.
*/
unsigned char findState(int x, int y);

/* placeTile
Places the corresponding tile of an item. Decreases the amount of the item if
successful.

x, y: Coordinates within the world.
item: Pointer to the item.
*/
void placeTile(int x, int y, Item *item);

/* removeTile
Removes a tile from the world and gives the player the appropriate item/s.

x, y: Coordinates of a tile within the world.
*/
void removeTile(int x, int y);

/* spawnEntity
Spawns the given entity at the given coordinates.

entity: The entity type to spawn.
x, y: Pixel coordinates of the position the entity should be spawned at.

Returns the index in the world entity slots of the entity if successful, or -1
if unsuccessful.
*/
int spawnEntity(enum Entities entity, int x, int y);

/* removeEntity
Removes the entity at the given slot index. Works even if there is no entity in
the slot, but will return false.

idx: Index of the entity to be removed.

Returns true if successful, false otherwise.
*/
bool removeEntity(int idx);

/* checkFreeEntitySlot
Checks the world entity slots to see if one is available.

Returns true if one is available, false if none are available.
*/
bool checkFreeEntitySlot();