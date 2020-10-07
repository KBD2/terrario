#pragma once

/*
----- SAVE -----

The game save/load system.
*/

#include "entity.h"

#define REGIONS_X (WORLD_WIDTH / REGION_SIZE + 1)
#define REGIONS_Y (WORLD_HEIGHT / REGION_SIZE + 1)
#define NUM_REGIONS REGIONS_X * REGIONS_Y

struct SaveData {
	int tileDataSize;
	int regionsX;
	int regionsY;
	unsigned char *tileData;
	char regionData[NUM_REGIONS];
	int timeTicks;
	int error;
};

struct PlayerSave {
	Item items[INVENTORY_SIZE];
	int health;
};

extern struct SaveData save;
extern struct Player player;

/* saveGame
Saves information on the player and changed regions to files.
*/
void saveGame();

/* loadSave
Loads save files.
*/
void loadSave();

/* getSave
Checks if a game save exists.

Returns true if a save exists, false otherwise.
*/
bool getSave();

/* getVersionInfo
Copies the \TERRARIO\save.info file into versionBuffer. Use getSave to ensure
the file exists!
*/
void getVersionInfo();