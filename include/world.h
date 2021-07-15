#pragma once

/*
----- WORLD -----

World API functions.
*/

#include <gint/display.h>
#include <stdbool.h>

#include "inventory.h"
#include "render.h"
#include "entity.h"
#include "chest.h"
#include "save.h"
#include "npc.h"

#define MAX_FRIENDS 8

enum SupportTypes {
	SUPPORT_NONE,
	SUPPORT_KEEP,
	SUPPORT_NEED,
	SUPPORT_TOP
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
	bool compress;
	float hitpoints;
	bool canFlood;
} TileData;

extern const TileData tiles[];

typedef struct {
//	Index in tiles array
	unsigned char id;
} Tile;

enum Tiles {
	TILE_NULL = -1,
	TILE_NOTHING,
	TILE_STONE,
	TILE_DIRT,
	TILE_GRASS,
	TILE_WOOD,
	TILE_TRUNK,
	TILE_ROOT_L, TILE_ROOT_R,
	TILE_LEAVES,
	TILE_PLANT,
	TILE_WBENCH_L, TILE_WBENCH_R,
	TILE_PLATFORM,
	TILE_CHAIR_L, TILE_CHAIR_R,
	TILE_TORCH,
	TILE_FURNACE_EDGE, TILE_FURNACE_MID,
	TILE_IRON_ORE,
	TILE_ANVIL_L, TILE_ANVIL_R,
	TILE_CHEST_L, TILE_CHEST_R,
	TILE_DOOR_C,
	TILE_DOOR_O_L_L, TILE_DOOR_O_L_R,
	TILE_DOOR_O_R_L, TILE_DOOR_O_R_R,
	TILE_VINE,
	TILE_SAND,
	TILE_CACTUS, TILE_CACTUS_BRANCH,
	TILE_WATER,
	TILE_CRYST_L,
	TILE_CRYST_R,

	TILE_MUD,
	TILE_CLAY,
	TILE_COPPER_ORE,
	TILE_TIN_ORE,
	TILE_MUSHROOM,
	TILE_BOTTLE,
	TILE_LESSER_HEALING_POTION,
	TILE_LESSER_MANA_POTION,

	TILES_COUNT
};

struct World {
	Tile *tiles;
	Entity *entities;
	int numNPCs;
	NPC *npcs;
	int numMarkers;
	HouseMarker *markers;
	struct ParticleExplosion explosion;
	int timeTicks;
	struct Chests chests;

	void (*placeTile)(int x, int y, Item *item);
	void (*removeTile)(int x, int y);
	int (*spawnEntity)(enum Entities entity, int x, int y);
	bool (*removeEntity)(int idx);
	bool (*checkFreeEntitySlot)();
};
	
extern struct World world;

typedef union {
	Tile tiles[4];
	uint32_t aligned;
} tilePun;

extern tilePun group;

static inline Tile getTile(int x, int y)
{
	int tileWanted = y * game.WORLD_WIDTH + x;
	if(game.HWMODE == MODE_RAM)
	{
		return world.tiles[tileWanted];
	}
	else
	{
		group.aligned = *(uint32_t *)(save.tileData + (tileWanted & ~3));
		return group.tiles[tileWanted & 3];
	}
}

static inline void setTile(int x, int y, enum Tiles tile)
{
	int tileWanted = y * game.WORLD_WIDTH + x;
	uint32_t *nearest;

	if(game.HWMODE == MODE_RAM)
	{
		world.tiles[tileWanted] = (Tile){tile};
		return;
	}
	else
	{
		nearest = save.tileData + (tileWanted & ~3);
		group.aligned = *nearest;
		group.tiles[tileWanted & 3] = (Tile){tile};
		*nearest = group.aligned;
		return;
	}
}

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

/* generateTree
Generates a tree.

x, y: Coordinates of the tree's base.
baseHeight: Height of the tree before random variation.
*/
void generateTree(int x, int y, int baseHeight);

/* breakTree
Breaks a tree, starting from the given trunk coordinate.

x, y: Coordinates of a TILE_TRUNK tile.
*/
void breakTree(int x, int y);

/* generateCactus
Generates a cactus.

x, y: Coordinates of the base of the cactus.
height: Height of the cactus.
*/
void generateCactus(int x, int y, int height);

/* breakCactus
Breaks a cactus, starting from the given stem coordinates.

x, y: Coordinates of a cactus tile.
*/
void breakCactus(int x, int y);

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

/* openDoor
Opens the door at the given position.

x, y: Coordinates of a closed door.
*/
void openDoor(int x, int y);

/* closeDoor
Closes the door at the given position.

x, y: Coordinates of an open door.
*/
void closeDoor(int x, int y);

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

/* timeToTicks
Returns the ticks for a given 24-hour time.

hour: The hour, from 0 to 23.
minute: The minute, from 0 to 59.

Returns the corresponding number of day ticks.*/
int timeToTicks(int hour, int minute);

/* getTime
Fills the given pointers with the hour and minute.

hour: Pointer to an hour variable.
minute: Pointer to a minute variable.
*/
void getTime(int *hour, int *minute);

/* checkArea
Performs a generic check that an object has sufficient space to be placed.

x, y: Coordinates of the top left of the area.
width, height: Size of the area.
support: Whether to check if the tiles below the area fully support the area.
*/
bool checkArea(int x, int y, int width, int height, bool support);

/*isDay
Returns true if it's currently daytime.
*/
bool isDay();
