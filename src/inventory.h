#pragma once

#include <stdbool.h>
#include <gint/display.h>

#include "defs.h"
#include "menu.h"

enum Items {
	ITEM_NULL,
	ITEM_STONE,
	ITEM_DIRT,
	ITEM_WOOD,

	ITEMS_COUNT
};

typedef struct {
	bopti_image_t* sprite;
	int maxStack;
	short tile;
	char* name;
} ItemData;

typedef struct {
	enum Items id;
	int number;
} Item;

extern const ItemData items[];

struct Inventory {
	Item items[INVENTORY_SIZE];
	int hotbarSlot;

	int (*getFirstFreeSlot)(enum Items item);
	void (*removeItem)(int slot);
	void (*stackItem)(Item* dest, Item* source);
};

int getFirstFreeSlot(enum Items item);
int findSlot(enum Items item);
void removeItem(int slot);
void stackItem(Item* dest, Item* source);