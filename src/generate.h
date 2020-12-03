#pragma once

/*
----- GENERATE -----

World generation system.
*/

#include "inventory.h"

typedef struct {
	int num;
	const enum Items *items;
	int amountMin;
	int amountMax;
	int ratioLow;
	int ratioHigh;
} ChestLoot;

struct ChestLootTable {
	int num;
	const ChestLoot *loot;
};

enum LootTables {
	TABLE_UNDERGROUND
};

/* generateWorld
Generates a world.
*/
void generateWorld();

/* addLoot
Adds loot to a chest from the given loot table.

chest: Pointer to the chest.
table: The table to use.
*/
void addLoot(struct Chest *chest, enum LootTables table);