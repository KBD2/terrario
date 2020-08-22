#pragma once

#include "inventory.h"
#include "world.h"

#define RECIPE_BUFFER_SIZE 128

struct Recipe {
	enum Tiles required;
	Item result;
	unsigned char numIngredients;
	const Item *ingredients;
};

extern const struct Recipe recipes[];

void craftingMenu();