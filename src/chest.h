#pragma once

#include "inventory.h"

struct Chests {
	int number;
	struct Chest *chests;

	bool (*addChest)(int x, int y);
	bool (*removeChest)(int x, int y);
	struct Chest *(*findChest)(int x, int y);
};

/* addChest
Adds a chest to the world chest database, if there is a free slot.

x, y: World coordinates of the top left tile of the chest.
*/
bool addChest(int x, int y);

/* removeChest
Removes a chest from the world database, if it's a valid chest.

x, y: World coordinates of the top left tile of the chest.
*/
bool removeChest(int x, int y);

/* findChest
Returns a pointer to a chest in the database.

x, y: World coordinates to search the database for.
*/
struct Chest *findChest(int x, int y);