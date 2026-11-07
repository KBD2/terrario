#pragma once

/*
----- SAVE -----

The game save/load system.

The world is infinite and generated chunk by chunk (see worldgen.h); only a
small window of chunks lives in RAM (see stream.h). Everything is stored in
the TERRARIO folder of the storage memory:

- save.info   Version, world seed and time of day.
- player.dat  Inventory, health, position and spawn point (global pixels).
- npc.dat     House markers and NPCs, in global coordinates.
- C*.dat      One file per chunk the player has modified. Unmodified chunks
              are not saved at all: they are regenerated from the seed.
- K*.dat      One file per chest that has been opened or placed.

File names are kept within 8 characters so they also work on the older
CASIOWIN filesystem (fx-9860GII). Every write has an even size.
*/

#include <stdbool.h>
#include <stdint.h>

#include "entity.h"

/* The one place where the storage memory folder is spelled out */
#define SAVE_FOLDER u"\\\\fls0\\TERRARIO"
#define SAVE_FILE(name) u"\\\\fls0\\TERRARIO\\" name
#define SAVE_FOLDER_ASCII "\\\\fls0\\TERRARIO\\"

struct SaveData {
	int tileDataSize;
	void *tileData;
	int timeTicks;
	int error;

	/* Seed shared by every chunk of the world */
	unsigned int worldSeed;
};

extern struct SaveData save;
extern struct Player player;

/* saveGame
Saves the player, NPCs and every modified chunk in RAM. Must be called inside
a gint_world_switch().
*/
void saveGame();

/* loadSave
Loads save.info and player.dat. Must be called inside a gint_world_switch().
The chunk window and NPCs are loaded afterwards with finishLoad().
*/
void loadSave();

/* finishLoad
Loads the chunk window around the player and then the NPCs. Called from the
normal gint world after a successful loadSave().
*/
void finishLoad();

/* getSave
Checks if a game save exists.

Returns true if a save exists, false otherwise.
*/
bool getSave();

/* ensureSaveFolder
Creates the TERRARIO folder if it does not exist. Must be called inside a
gint_world_switch().
*/
bool ensureSaveFolder();

/* clearSaveFolder
Deletes the chunk and chest files of a previous world. Must be called inside a
gint_world_switch().
*/
void clearSaveFolder();

/* getVersionInfo
Copies the version string of save.info into versionBuffer. Use getSave to
ensure the file exists!
*/
void getVersionInfo();

/* savePath
Builds the FONTCHARACTER path of a file in the save folder.

name: ASCII file name, at most 12 characters.
path: Receives the path, must hold at least 40 characters.
*/
void savePath(const char *name, uint16_t *path);

/* encodeName
Writes a value as `digits` base-32 characters (0-9, A-V). Used to keep file
names short.
*/
void encodeName(char *out, unsigned int value, int digits);
