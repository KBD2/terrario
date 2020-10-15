#pragma once

#include "inventory.h"

struct Chests {
	int number;
	struct Chest *chests;

	bool (*addChest)(int x, int y);
	bool (*removeChest)(int x, int y);
	struct Chest *(*findChest)(int x, int y);
};

bool addChest(int x, int y);

bool removeChest(int x, int y);

struct Chest *findChest(int x, int y);