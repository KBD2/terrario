#pragma once

/*
----- SAVE -----

The game save/load system.

The world is split into 160x75 chunks. Only the chunk the player is currently
in lives in RAM; every chunk that has been visited is stored in its own file
under \\fls0\TERRARIO. Moving to an adjacent chunk saves the current one first,
then loads the neighbouring chunk if it has been visited before, or generates
it if it has not.
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

	/* Global position of the loaded chunk, in chunks. */
	int chunkX;
	int chunkY;

	/* Seed for the terrain heightmap, shared by every chunk of the world. */
	unsigned int worldSeed;

	/* Set when loadChunkFile() could not find the requested chunk file. */
	bool chunkMissing;
};

extern struct SaveData save;
extern struct Player player;

/* saveGame
Saves the player and the currently loaded chunk to files.
*/
void saveGame();

/* loadSave
Loads the save files.
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

/* travelToChunk
Saves the current chunk, then loads the chunk at the given coordinates (or
generates it if this is the first visit) and puts the player at the given
chunk-local pixel position.

Returns true on success, false if saving or loading failed.
*/
bool travelToChunk(int chunkX, int chunkY, int playerX, int playerY);

/* loadChunkFile
Loads the chunk at save.chunkX/save.chunkY. If the file does not exist,
save.chunkMissing is set and no chunk data is changed. Must be called inside a
gint_world_switch().
*/
void loadChunkFile();
