#pragma once

#include <stdbool.h>
#include <gint/display.h>

enum Items {
	ITEM_NULL = -1,
	ITEM_STONE,
	ITEM_DIRT
};

typedef struct {
	bopti_image_t* sprite;
	int maxStack;
	short tile;
} ItemData;

typedef struct {
	enum Items id;
	int number;
} Item;

extern const ItemData items[];

struct Inventory {
	Item items[10];
};