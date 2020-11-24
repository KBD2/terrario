#pragma once

/*
----- SAVE -----

The game save/load system.
*/

#include "entity.h"

struct SaveData {
	int tileDataSize;
	int regionsX;
	int regionsY;
	void *tileData;
	char regionData[MAX_REGIONS];
	int timeTicks;
	int error;
};

struct PlayerSave {
	Item items[INVENTORY_SIZE];
	Item accessories[5];
	Item armour[3];
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