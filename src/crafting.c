#include <gint/std/stdlib.h>
#include <gint/std/string.h>
#include <gint/keyboard.h>
#include <gint/gray.h>
#include <gint/timer.h>
#include <stdbool.h>
#include <gint/gint.h>
#include <gint/defs/util.h>

#include "crafting.h"
#include "entity.h"
#include "world.h"
#include "render.h"

const struct Recipe recipes[] = {
//		Workbench		Result				N	Ingredient list
//	{	TILE_WBENCH_L,	{ITEM_WOOD, 2},		1,	(const Item[]){	{ITEM_WOOD,	1}		}	},
	{	TILE_NULL,		{ITEM_PLATFORM, 2},	1,	(const Item[]){	{ITEM_WOOD, 1}		}	},
	{	TILE_NULL,		{ITEM_WOOD, 1},		1,	(const Item[]){	{ITEM_PLATFORM, 2}	}	},
	{	TILE_NULL,		{ITEM_WBENCH, 1},	1,	(const Item[]){	{ITEM_WOOD,	10}		}	},
	{	TILE_WBENCH_L,	{ITEM_CHAIR, 1},	1,	(const Item[]){	{ITEM_WOOD, 4}		}	}
};

bool *findNearTiles()
{
	bool *buffer = (bool*)malloc(TILES_COUNT * sizeof(bool));
	int playerTileX = player.props.x >> 3;
	int playerTileY = player.props.y >> 3;
	int checkX, checkY;

	memset(buffer, 0, TILES_COUNT * sizeof(bool));

	for(enum Tiles tile = TILE_NOTHING; tile < TILES_COUNT; tile++)
	{
		for(int dY = -4; dY < 6; dY++)
		{
			for(int dX = -4; dX < 5; dX++)
			{
				checkX = playerTileX + dX;
				checkY = playerTileY + dY;
				if(checkX < 0 || checkX >= WORLD_WIDTH || checkY < 0 || checkY >= WORLD_HEIGHT) continue;
				if(getTile(checkX, checkY).idx == tile)
				{
					buffer[tile] = true;
				}
			}
		}
	}

	return buffer;
}

bool checkRecipeIsCraftable(int recipe)
{
	const struct Recipe *checkRecipe = &recipes[recipe];
	bool craftable = true;

	for(int ingredient = 0; ingredient < checkRecipe->numIngredients; ingredient++)
	{
		if(player.inventory.tallyItem(checkRecipe->ingredients[ingredient].id) < checkRecipe->ingredients[ingredient].number)
		{
			craftable = false;
			break;
		}
	}

	return craftable;
}

void findCraftableRecipes(short *buffer)
{
	const struct Recipe *currRecipe;
	bool craftable;
	bool *nearTiles = findNearTiles();
	int count = 0;

	for(int i = 0; i < RECIPE_BUFFER_SIZE; i++) buffer[i] = -1;

	for(unsigned int recipe = 0; recipe < sizeof(recipes) / sizeof(struct Recipe); recipe++)
	{
		currRecipe = &recipes[recipe];
		if(currRecipe->required != TILE_NULL && !nearTiles[currRecipe->required]) continue;
		craftable = checkRecipeIsCraftable(recipe);
		if(craftable)
		{
			buffer[count] = recipe;
			count++;
			if(count == RECIPE_BUFFER_SIZE) break;
		}
	}

	free(nearTiles);
}

void craftingMenu()
{
	int selected = 0;
	int currCraftable;
	short *craftableRecipes = (short*)malloc(RECIPE_BUFFER_SIZE * sizeof(short));
	extern bopti_image_t img_slot, img_hotbarselect;
	key_event_t key;
	int recipesMax = 0;
	const Item *currIngredient;
	int numLeft;
	int slot;
	Item result;

	findCraftableRecipes(craftableRecipes);

	while(true)
	{
		dclear(C_WHITE);
		for(int recipe = 0; recipe < RECIPE_BUFFER_SIZE; recipe++)
		{
			currCraftable = craftableRecipes[recipe];
			if(currCraftable == -1) 
			{
				recipesMax = max(0, recipe - 1);
				break;
			}
			if(recipe - selected < -4 || recipe - selected > 4) continue;
			dimage((recipe - selected + 3) * 17 + 5, 1, &img_slot);
			renderItem((recipe - selected + 3) * 17 + 6, 2, &recipes[currCraftable].result);
		}
		currCraftable = craftableRecipes[selected];
		if(currCraftable != -1)
		{
			for(int i = 0; i < recipes[currCraftable].numIngredients; i++)
			{
				currIngredient = &recipes[currCraftable].ingredients[i];
				dimage(i * 16 + 1, 18, &img_slot);
				renderItem(i * 16 + 2, 19, currIngredient);
				dimage(i * 16 + 1, 42, &img_slot);
				renderItem(i * 16 + 2, 43, &(Item){currIngredient->id, player.inventory.tallyItem(currIngredient->id)});
			}
		}

		dimage(56, 1, &img_hotbarselect);
		dtext(1, 36, C_BLACK, "You have:");
		dupdate();

		key = getkey_opt(GETKEY_REP_ALL, NULL);
		switch(key.key)
		{
			case KEY_SHIFT: case KEY_ALPHA:
				if(key.type == KEYEV_DOWN)
				{
					free(craftableRecipes);
					return;
				}
				break;

			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_switch(&takeVRAMCapture);
				break;

			case KEY_EXE:
				currCraftable = craftableRecipes[selected];
				if(currCraftable == -1) break;
				for(int i = 0; i < recipes[currCraftable].numIngredients; i++)
				{
					currIngredient = &recipes[currCraftable].ingredients[i];
					numLeft = currIngredient->number;
					while(numLeft > 0)
					{
						slot = player.inventory.findSlot(currIngredient->id);
						if(player.inventory.items[slot].number > numLeft)
						{
							player.inventory.items[slot].number -= numLeft;
							numLeft = 0;
						}
						else
						{
							numLeft -= player.inventory.items[slot].number;
							player.inventory.items[slot] = (Item){ITEM_NULL, 0};
						}
						
					}
				}
				result = recipes[currCraftable].result;
				while(result.id != ITEM_NULL)
				{
					slot = player.inventory.getFirstFreeSlot(recipes[currCraftable].result.id);
					player.inventory.stackItem(&player.inventory.items[slot], &result);
				}

// 				Regen valid recipe buffer as multiple may have changed
				findCraftableRecipes(craftableRecipes); 
				for(int i = 0; i < RECIPE_BUFFER_SIZE; i++) 
				{
					if(craftableRecipes[i] == -1)
					{
						recipesMax = max(0, i - 1);
						break;
					}
				}

				break;

			case KEY_LEFT:
				selected--;
				break;
			case KEY_RIGHT:
				selected++;
				break;

			default:
				break;
		}
		selected = min(max(0, selected), recipesMax);
	}
}