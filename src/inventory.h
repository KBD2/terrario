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
	ITEM_WBENCH,
	ITEM_PLATFORM,
	ITEM_CHAIR,
	ITEM_SWORD,
	ITEM_GEL,

	ITEMS_COUNT
};

typedef struct {
	bopti_image_t *sprite;
	int maxStack;
	short tile;
	char *name;
	bool canSwing;
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
	void (*stackItem)(Item *dest, Item *source);
	int (*tallyItem)(enum Items item);
	int (*findSlot)(enum Items item);
	Item *(*getSelected)();
};

int getFirstFreeSlot(enum Items item);
int findSlot(enum Items item);
void removeItem(int slot);
void stackItem(Item *dest, Item *source);
int tallyItem(enum Items item);
Item *getSelected();

void inventoryMenu();