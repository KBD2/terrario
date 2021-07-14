#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <gint/keyboard.h>
#include <gint/gray.h>
#include <gint/timer.h>
#include <gint/gint.h>
#include <gint/defs/util.h>

#include "crafting.h"
#include "entity.h"
#include "world.h"
#include "menu.h"
#include "defs.h"
#include "render.h"

const struct Recipe recipes[] = {
//		Workbench			Result											N	Ingredient list
	{	TILE_NULL,			{ITEM_PLATFORM, PREFIX_NONE, 2},				1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 1}																			}	},
	{	TILE_NULL,			{ITEM_WOOD, PREFIX_NONE, 1},					1,	(const Item[]){ {ITEM_PLATFORM, PREFIX_NONE, 2}																		}	},
	{	TILE_NULL,			{ITEM_WBENCH, PREFIX_NONE, 1},					1,	(const Item[]){ {ITEM_WOOD,	PREFIX_NONE, 10}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CHAIR, PREFIX_NONE, 1},					1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 4}																			}	},
	{	TILE_NULL,			{ITEM_TORCH, PREFIX_NONE, 3},					2,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 1},		{ITEM_GEL, PREFIX_NONE, 1}										}	},
	{	TILE_WBENCH_L,		{ITEM_FURNACE, PREFIX_NONE, 1},					3,	(const Item[]){ {ITEM_STONE, PREFIX_NONE, 20},		{ITEM_WOOD, PREFIX_NONE, 4},	{ITEM_TORCH, PREFIX_NONE, 3}	}	},
	{	TILE_FURNACE_MID,	{ITEM_IRON_BAR, PREFIX_NONE, 1}, 				1,	(const Item[]){ {ITEM_IRON_ORE, PREFIX_NONE, 3}																		}	},
	{	TILE_WBENCH_L,		{ITEM_ANVIL, PREFIX_NONE, 1},					1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 5}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CHEST, PREFIX_NONE, 1},					2,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 8},		{ITEM_IRON_BAR, PREFIX_NONE, 2}									}	},
	{	TILE_WBENCH_L,		{ITEM_DOOR, PREFIX_NONE, 1},					1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 6}																			}	},
// Iron armour cost is about 50% of original
	{	TILE_ANVIL_L,		{ITEM_IRON_HELMET, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 8}																		}	},//15
	{	TILE_ANVIL_L,		{ITEM_IRON_CHAINMAIL, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 16}																	}	},//25
	{	TILE_ANVIL_L,		{ITEM_IRON_GREAVES, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 10}																	}	},//20
	{	TILE_WBENCH_L,		{ITEM_WOOD_HELMET, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 20}																		}	},
	{	TILE_WBENCH_L,		{ITEM_WOOD_BREASTPLATE, PREFIX_NONE, 1},		1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 30}																		}	},
	{	TILE_WBENCH_L,		{ITEM_WOOD_GREAVES, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_WOOD, PREFIX_NONE, 25}																		}	},
	{	TILE_ANVIL_L,		{ITEM_IRON_SWORD, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 8}																		}	},
	{	TILE_ANVIL_L,		{ITEM_IRON_PICK, PREFIX_NONE, 1},				2,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 12},	{ITEM_WOOD, PREFIX_NONE, 3}										}	},
	{	TILE_ANVIL_L,		{ITEM_EMPTY_BUCKET, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_IRON_BAR, PREFIX_NONE, 3}																		}	},

	{	TILE_FURNACE_MID,	{ITEM_COPPER_BAR, PREFIX_NONE, 1}, 				1,	(const Item[]){ {ITEM_COPPER_ORE, PREFIX_NONE, 3}																	}	},
	{	TILE_FURNACE_MID,	{ITEM_TIN_BAR, PREFIX_NONE, 1}, 				1,	(const Item[]){ {ITEM_TIN_ORE, PREFIX_NONE, 3}																		}	},
	{	TILE_ANVIL_L,		{ITEM_TIN_HELMET, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_TIN_BAR, PREFIX_NONE, 8}																		}	},//15
	{	TILE_ANVIL_L,		{ITEM_TIN_CHAINMAIL, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_TIN_BAR, PREFIX_NONE, 16}																		}	},//25
	{	TILE_ANVIL_L,		{ITEM_TIN_GREAVES, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_TIN_BAR, PREFIX_NONE, 10}																		}	},//20
	{	TILE_ANVIL_L,		{ITEM_COPPER_SWORD, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_COPPER_BAR, PREFIX_NONE, 8}																	}	},
	{	TILE_ANVIL_L,		{ITEM_COPPER_PICK, PREFIX_NONE, 1},				2,	(const Item[]){ {ITEM_COPPER_BAR, PREFIX_NONE, 12},	{ITEM_WOOD, PREFIX_NONE, 3}										}	},
	{	TILE_ANVIL_L,		{ITEM_TIN_SWORD, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_TIN_BAR, PREFIX_NONE, 8}																		}	},
	{	TILE_ANVIL_L,		{ITEM_TIN_PICK, PREFIX_NONE, 1},				2,	(const Item[]){ {ITEM_TIN_BAR, PREFIX_NONE, 12},	{ITEM_WOOD, PREFIX_NONE, 3}										}	},
	{	TILE_NULL,			{ITEM_COIN_COPPER, PREFIX_NONE, 100},			1,	(const Item[]){ {ITEM_COIN_SILVER, PREFIX_NONE, 1}																	}	},
	{	TILE_NULL,			{ITEM_COIN_SILVER, PREFIX_NONE, 100},			1,	(const Item[]){ {ITEM_COIN_GOLD, PREFIX_NONE, 1}																	}	},
	{	TILE_NULL,			{ITEM_COIN_GOLD, PREFIX_NONE, 100},				1,	(const Item[]){ {ITEM_COIN_PLATINUM, PREFIX_NONE, 1}																}	},
	{	TILE_NULL,			{ITEM_COIN_SILVER, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_COIN_COPPER, PREFIX_NONE, 100}																}	},
	{	TILE_NULL,			{ITEM_COIN_GOLD, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_COIN_SILVER, PREFIX_NONE, 100}																}	},
	{	TILE_NULL,			{ITEM_COIN_PLATINUM, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_COIN_GOLD, PREFIX_NONE, 100}																	}	},
	{	TILE_WBENCH_L,		{ITEM_CACTUS_HELMET, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_CACTUS, PREFIX_NONE, 20}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CACTUS_BREASTPLATE, PREFIX_NONE, 1},		1,	(const Item[]){ {ITEM_CACTUS, PREFIX_NONE, 30}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CACTUS_GREAVES, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_CACTUS, PREFIX_NONE, 25}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CACTUS_SWORD, PREFIX_NONE, 1},			1,	(const Item[]){ {ITEM_CACTUS, PREFIX_NONE, 10}																		}	},
	{	TILE_WBENCH_L,		{ITEM_CACTUS_PICK, PREFIX_NONE, 1},				1,	(const Item[]){ {ITEM_CACTUS, PREFIX_NONE, 15}																		}	},
	{	TILE_FURNACE_MID,	{ITEM_GLASS, PREFIX_NONE, 1}, 					1,	(const Item[]){ {ITEM_SAND, PREFIX_NONE, 2}																			}	},
	{	TILE_FURNACE_MID,	{ITEM_BOTTLE, PREFIX_NONE, 2}, 					1,	(const Item[]){ {ITEM_GLASS, PREFIX_NONE, 1}																		}	},
	{	TILE_BOTTLE,		{ITEM_LESSER_HEALING_POTION, PREFIX_NONE, 2},	3,	(const Item[]){ {ITEM_BOTTLE, PREFIX_NONE, 2}, {ITEM_GEL, PREFIX_NONE, 2}, {ITEM_MUSHROOM, PREFIX_NONE, 1}			}	},
};

const int numRecipes = sizeof(recipes) / sizeof(struct Recipe);

void findNearTiles(bool *tilesBuffer)
{
	int playerTileX = player.props.x >> 3;
	int playerTileY = player.props.y >> 3;
	int checkX, checkY;

	memset(tilesBuffer, 0, TILES_COUNT * sizeof(bool));

	for(enum Tiles tile = TILE_NOTHING; tile < TILES_COUNT; tile++)
	{
		for(int dY = -4; dY < 6; dY++)
		{
			for(int dX = -4; dX < 5; dX++)
			{
				checkX = playerTileX + dX;
				checkY = playerTileY + dY;
				if(checkX < 0 || checkX >= game.WORLD_WIDTH || checkY < 0 || checkY >= game.WORLD_HEIGHT) continue;
				if(getTile(checkX, checkY).id == tile)
				{
					tilesBuffer[tile] = true;
				}
			}
		}
	}
}

bool checkRecipeIsCraftable(int recipe)
{
	const struct Recipe *checkRecipe = &recipes[recipe];
	bool craftable = true;

	for(int ingredient = 0; ingredient < checkRecipe->numIngredients; ingredient++)
	{
		if(player.inventory.tallyItem(checkRecipe->ingredients[ingredient].id) < checkRecipe->ingredients[ingredient].amount)
		{
			craftable = false;
			break;
		}
	}

	return craftable;
}

void findCraftableRecipes(bool *tilesBuffer, short *craftableRecipes)
{
	const struct Recipe *currRecipe;
	bool craftable;
	int count = 0;

	findNearTiles(tilesBuffer);

	for(int i = 0; i < RECIPE_BUFFER_SIZE; i++) craftableRecipes[i] = -1;

	for(unsigned int recipe = 0; recipe < numRecipes; recipe++)
	{
		currRecipe = &recipes[recipe];
		if(currRecipe->required != TILE_NULL && !tilesBuffer[currRecipe->required]) continue;
		craftable = checkRecipeIsCraftable(recipe);
		if(craftable)
		{
			craftableRecipes[count] = recipe;
			count++;
			if(count == RECIPE_BUFFER_SIZE) break;
		}
	}
}

void craftingMenu()
{
	int selected = 0;
	int currCraftable;
	extern bopti_image_t img_ui_slots, img_ui_slot_highlight;
	key_event_t key;
	int recipesMax = 0;
	const Item *currIngredient;
	int numLeft;
	int slot;
	Item result;
	enum Items crafting = ITEM_NULL;

	bool tilesBuffer[TILES_COUNT];
	short craftableRecipes[RECIPE_BUFFER_SIZE];

	findCraftableRecipes(tilesBuffer, craftableRecipes);

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
			dsubimage((recipe - selected + 3) * 17 + 5, 0, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NOCLIP);
			renderItem((recipe - selected + 3) * 17 + 6, 1, (Item *)&recipes[currCraftable].result);
		}
		currCraftable = craftableRecipes[selected];
		if(currCraftable != -1)
		{
			dtext(1, 18, C_BLACK, items[recipes[currCraftable].result.id].name);
			for(int i = 0; i < recipes[currCraftable].numIngredients; i++)
			{
				currIngredient = &recipes[currCraftable].ingredients[i];
				dsubimage(i * 16 + 1, 23, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NOCLIP);
				renderItem(i * 16 + 2, 24, (Item *)currIngredient);
				dsubimage(i * 16 + 1, 46, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NOCLIP);
				renderItem(i * 16 + 2, 47, &(Item){currIngredient->id, PREFIX_NONE, player.inventory.tallyItem(currIngredient->id)});
			}
		}

		dimage(56, 0, &img_ui_slot_highlight);
		dtext(1, 41, C_BLACK, "You have:");
		dupdate();

		key = getkey_opt(GETKEY_REP_ALL, NULL);
		switch(key.key)
		{
			case KEY_SHIFT: case KEY_ALPHA:
				if(key.type == KEYEV_DOWN) return;
				break;

			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_world_switch(GINT_CALL(&takeVRAMCapture));
				break;

			case KEY_EXE:
				currCraftable = craftableRecipes[selected];
				if(currCraftable == -1) break;
				if(key.type == KEYEV_HOLD && crafting != recipes[currCraftable].result.id) break;
				crafting = recipes[currCraftable].result.id;
#ifndef DEBUGMODE
				for(int i = 0; i < recipes[currCraftable].numIngredients; i++)
				{
					currIngredient = &recipes[currCraftable].ingredients[i];
					numLeft = currIngredient->amount;
					while(numLeft > 0)
					{
						slot = player.inventory.findSlot(currIngredient->id);
						if(player.inventory.items[slot].amount > numLeft)
						{
							player.inventory.items[slot].amount -= numLeft;
							numLeft = 0;
						}
						else
						{
							numLeft -= player.inventory.items[slot].amount;
							player.inventory.items[slot] = (Item){ITEM_NULL, PREFIX_NONE, 0};
						}
					}
				}
#endif
				result = recipes[currCraftable].result;

				if (items[result.id].type != TOOL_TYPE_NONE)
				{
					result.prefix = rand() % PREFIX_COUNT;
				}

				while(result.id != ITEM_NULL)
				{
					slot = player.inventory.getFirstFreeSlot(recipes[currCraftable].result.id);
					player.inventory.stackItem(&player.inventory.items[slot], &result);
				}

// 				Regen valid recipe buffer as multiple may have changed
				findCraftableRecipes(tilesBuffer, craftableRecipes);
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
