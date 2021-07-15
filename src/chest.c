#include <stdlib.h>

#include "chest.h"
#include "world.h"

bool addChest(int x, int y)
{
	int last = world.chests.number;

	if(world.chests.number == MAX_CHESTS) return false;

	world.chests.number++;

	world.chests.chests[last] = (struct Chest){.topX = x, .topY = y};
	for(int slot = 0; slot < INVENTORY_SIZE; slot++) world.chests.chests[last].items[slot] = (Item){ITEM_NULL, PREFIX_NONE, 0};

	return true;
}

bool removeChest(int x, int y)
{
	int idx;
	struct Chest *chest = world.chests.findChest(x, y);

	if(chest == NULL) return false;
	world.chests.number--;
	idx = chest - world.chests.chests;
	world.chests.chests[idx] = world.chests.chests[world.chests.number];
	
	return true;
}

struct Chest *findChest(int x, int y)
{
	struct Chest *check;

	for(int chest = 0; chest < world.chests.number; chest++)
	{
		check = &world.chests.chests[chest];
		if(check->topX == x && check->topY == y) return check;
	}

	return NULL;
}
