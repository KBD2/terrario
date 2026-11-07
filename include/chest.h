#pragma once

#include "inventory.h"

/*
Chest contents live in one small file per chest. Only the chest currently
being interacted with is kept in RAM; addChest/findChest/removeChest touch the
filesystem on demand.
*/
struct Chests {
	struct Chest active;
	bool activeValid;

	bool (*addChest)(int x, int y);
	bool (*removeChest)(int x, int y);
	struct Chest *(*findChest)(int x, int y);
	bool (*flushChest)();
	void (*closeChest)();
};

/* addChest
Creates an empty chest record at the given top-left tile, or loads it if it
already exists.

x, y: World coordinates of the top left tile of the chest.
*/
bool addChest(int x, int y);

/* removeChest
Deletes the chest record at the given top-left tile.

x, y: World coordinates of the top left tile of the chest.
*/
bool removeChest(int x, int y);

/* findChest
Loads the chest at the given top-left tile into the single active chest slot.

x, y: World coordinates to search for.

Returns a pointer to the active chest, or NULL if there is no chest there.
*/
struct Chest *findChest(int x, int y);

/* flushChest
Writes the active chest back to its file, if one is loaded.
*/
bool flushChest();

/* closeChest
Drops the active chest from RAM without writing it.
*/
void closeChest();
