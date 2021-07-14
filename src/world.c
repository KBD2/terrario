#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <gint/defs/util.h>

#include "world.h"
#include "defs.h"
#include "save.h"

#define PI 3.14159265358979323846

extern bopti_image_t 
img_tiles_nothing,
img_tiles_stone,
img_tiles_dirt,
img_tiles_grass,
img_tiles_wood,
img_tiles_trunk,
img_tiles_root_l, img_tiles_root_r,
img_tiles_plant,
img_tiles_wbench,
img_tiles_platform,
img_tiles_chair,
img_tiles_torch,
img_tiles_furnace_edge, img_tiles_furnace_mid,
img_tiles_iron_ore,
img_tiles_anvil,
img_tiles_chest,
img_tiles_door_c,
img_tiles_door_o_l_l, img_tiles_door_o_l_r,
img_tiles_door_o_r_l, img_tiles_door_o_r_r,
img_tiles_vine,
img_tiles_sand,
img_tiles_cactus,
img_tiles_water,
img_tiles_cryst,
img_tiles_mud,
img_tiles_clay,
img_tiles_copper_ore,
img_tiles_tin_ore,
img_tiles_mushroom,
img_tiles_bottle,
img_tiles_lesser_healing_potion,
img_tiles_lesser_mana_potion;

const TileData tiles[] = {
//      Ptr to sprite       				Phys?			Render?	Type?			Support?		Friends (-1 to end)								   								Item						Name						Cmprs?	HP		Floodable?
	{	&img_tiles_nothing,					PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1},																			ITEM_NULL,					"Nothing",					true,	999,	true	},	// TILE_NOTHING
	{   &img_tiles_stone,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_IRON_ORE, TILE_CLAY, TILE_MUD, -1},							ITEM_STONE,					"Stone",					true,	1.0,	false	},	// TILE_STONE
	{   &img_tiles_dirt ,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_STONE, TILE_GRASS, TILE_IRON_ORE, TILE_SAND, TILE_CLAY, TILE_MUD, -1},	ITEM_DIRT,					"Dirt",						true,	0.5,	false	},	// TILE_DIRT
	{	&img_tiles_grass,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, -1},																ITEM_NULL,					"Grass",					false,	0.1,	false	},	// TILE_GRASS
	{	&img_tiles_wood,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{-1},																			ITEM_WOOD,					"Wood",						false,	1.0,	false	},	// TILE_WOOD
	{	&img_tiles_trunk,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_KEEP,	{TILE_ROOT_L, TILE_ROOT_R, TILE_LEAVES, -1},									ITEM_WOOD,					"Tree Trunk",				false,	5.0,	false	},	// TILE_TRUNK
	{	&img_tiles_root_l,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1},																			ITEM_WOOD,					"Tree Root",				false,	5.0,	false	},	// TILE_ROOT_L
	{	&img_tiles_root_r,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1},																			ITEM_WOOD,					"Tree Root",				false,	5.0,	false	},	// TILE_ROOT_R
	{	&img_tiles_nothing,					PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1},																			ITEM_WOOD,					"Tree Top",					false,	5.0,	false	},	// TILE_LEAVES
	{	&img_tiles_plant,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_NULL,					"Plant"	,					false,	0.1,	true	},	// TILE_PLANT
	{	&img_tiles_wbench,					PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_WBENCH,				"Workbench",				false,	0.1,	true	},	// TILE_WBENCH_L
	{	&img_tiles_wbench,					PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_WBENCH,				"Workbench R",				false,	0.1,	true	},	// TILE_WBENCH_R
	{	&img_tiles_platform,				PHYS_PLATFORM,	true,	TYPE_SHEET,		SUPPORT_NONE,	{-1},																			ITEM_PLATFORM,				"Platform",					false,	0.1,	true	},	// TILE_PLATFORM
	{	&img_tiles_chair,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_CHAIR,					"Chair L",					false,	0.1,	true	},	// TILE_CHAIR_L
	{	&img_tiles_chair,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_CHAIR,					"Chair R",					false,	0.1,	true	},	// TILE_CHAIR_R
	{	&img_tiles_torch,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_NOTHING, -1},																ITEM_TORCH,					"Torch",					false,	0.1,	true	},	// TILE_TORCH
	{	&img_tiles_furnace_edge,			PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_FURNACE,				"Furnace",					false,	0.1,	true	},	// TILE_FURNACE_EDGE
	{	&img_tiles_furnace_mid,				PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_FURNACE,				"Furnace",					false,	0.1,	true	},	// TILE_FURNACE_MID
	{	&img_tiles_iron_ore,				PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_STONE, -1},													ITEM_IRON_ORE,				"Iron Ore",					false,	1.0,	false	},	// TILE_IRON_ORE
	{	&img_tiles_anvil,					PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_ANVIL,					"Anvil",					false,	0.1,	true	},	// TILE_ANVIL_L
	{	&img_tiles_anvil,					PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_ANVIL,					"Anvil R",					false,	0.1,	true	},	// TILE_ANVIL_R
	{	&img_tiles_chest,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1},																			ITEM_CHEST,					"Chest L",					false,	0.1,	true	},	// TILE_CHEST_L
	{	&img_tiles_chest,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1},																			ITEM_CHEST,					"Chest R",					false,	0.1,	true	},	// TILE_CHEST_R
	{	&img_tiles_door_c,					PHYS_SOLID,		true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_DOOR,					"Door C",					false,	0.1,	false	},	// TILE_DOOR_C
	{	&img_tiles_door_o_l_l,				PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NONE,	{-1},																			ITEM_DOOR,					"Door O L L",				false,	0.1,	true	},	// TILE_DOOR_O_L_L
	{	&img_tiles_door_o_l_r,				PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_DOOR,					"Door O L R",				false,	0.1,	true	},	// TILE_DOOR_O_L_R
	{	&img_tiles_door_o_r_l,				PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_DOOR,					"Door O R L",				false,	0.1,	true	},	// TILE_DOOR_O_R_L
	{	&img_tiles_door_o_r_r,				PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NONE,	{-1},																			ITEM_DOOR,					"Door O R R",				false,	0.1,	true	},	// TILE_DOOR_O_R_R
	{	&img_tiles_vine,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_TOP,	{-1},																			ITEM_NULL,					"Vine",						false,	0.1,	true	},	// TILE_VINE
	{	&img_tiles_sand,					PHYS_SAND,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, -1},																ITEM_SAND,					"Sand",						true,	0.5,	false	},	// TILE_SAND
	{	&img_tiles_cactus,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_KEEP,	{TILE_CACTUS_BRANCH, -1},														ITEM_CACTUS,				"Cactus",					false,	0.5,	true	},	// TILE_CACTUS
	{	&img_tiles_cactus,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_KEEP,	{TILE_CACTUS, -1},																ITEM_CACTUS,				"Cactus",					false,	0.5,	true	},	// TILE_CACTUS_BRANCH
	{	&img_tiles_water,					PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{-1},																			ITEM_NULL,					"Water",					true,	999,	false	},	// TILE_WATER
	{	&img_tiles_cryst,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_CRYST,					"Life Crystal",				false,	1.0,	false	},	// TILE_CRYST_L
	{	&img_tiles_cryst,					PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1},																			ITEM_CRYST,					"Life Crystal",				false,	1.0,	false	},	// TILE_CRYST_R

	{   &img_tiles_mud,						PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_STONE, TILE_DIRT, TILE_CLAY, -1},											ITEM_MUD,					"Mud",						true,	0.5,	false	},	// TILE_MUD
	{   &img_tiles_clay,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_STONE, TILE_DIRT, TILE_MUD, -1},											ITEM_CLAY,					"Clay",						true,	0.5,	false	},	// TILE_CLAY
	{	&img_tiles_copper_ore,				PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_STONE, -1},													ITEM_COPPER_ORE,			"Copper Ore",				false,	0.8,	false	},	// TILE_COPPER_ORE
	{	&img_tiles_tin_ore,					PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_STONE, -1},													ITEM_TIN_ORE,				"Tin Ore",					false,	0.8,	false	},	// TILE_TIN_ORE
	{	&img_tiles_mushroom,				PHYS_NON_SOLID,	true,	TYPE_TILE,		SUPPORT_NEED,	{-1},																			ITEM_MUSHROOM,				"Mushroom",					false,	0.1,	true	},	// TILE_MUSHROOM
	{	&img_tiles_bottle,					PHYS_NON_SOLID,	true,	TYPE_TILE,		SUPPORT_NEED,	{-1},																			ITEM_BOTTLE,				"Bottle",					false,	0.1,	true	},	// TILE_BOTTLE
	{	&img_tiles_lesser_healing_potion,	PHYS_NON_SOLID,	true,	TYPE_TILE,		SUPPORT_NEED,	{-1},																			ITEM_LESSER_HEALING_POTION,	"Lesser Healing Potion"	,	false,	0.1,	true	},	// TILE_LESSER_HEALING_POTION
	{	&img_tiles_lesser_mana_potion,		PHYS_NON_SOLID,	true,	TYPE_TILE,		SUPPORT_NEED,	{-1},																			ITEM_LESSER_MANA_POTION,	"Lesser Mana Potion",		false,	0.1,	true	},	// TILE_LESSER_MANA_POTION
};

tilePun group;

int timeToTicks(int hour, int minute)
{
	hour %= 24;
	minute %= 60;

	return (DAY_TICKS / 24) * hour + (DAY_TICKS / 1440) * minute;
}

void getTime(int *hour, int *minute)
{
	*hour = (float)world.timeTicks / DAY_TICKS * 24;
	*minute = (world.timeTicks % (DAY_TICKS / 24)) / (DAY_TICKS / 1440);

	return;
}

unsigned char makeVar()
{
	return rand() % 3;
}

void generateTree(int x, int y, int baseHeight)
{
	int top;
	int height = max(1, baseHeight + (rand() % 3) - 1);

	if(x == 0 || x == game.WORLD_WIDTH - 1 || y < 0 || y >= game.WORLD_HEIGHT - height) return;
	for(int i = 0; i < height; i++) if(
		getTile(x - 1, y - i).id == TILE_TRUNK
	 || getTile(x + 1, y - i).id == TILE_TRUNK
	 || getTile(x - 2, y - i).id == TILE_TRUNK
	 || getTile(x + 2, y - i).id == TILE_TRUNK
	 ) return;

	for(top = 0; top < height; top++)
	{
		setTile(x, y - top, TILE_TRUNK);
	}
	setTile(x, y - top, TILE_LEAVES);
	if(getTile(x - 1, y + 1).id == TILE_GRASS && tiles[getTile(x - 1, y).id].physics == PHYS_NON_SOLID && rand() % 3 <= 1)
	{
		setTile(x - 1, y, TILE_ROOT_L);
	} 
	if(getTile(x + 1, y + 1).id == TILE_GRASS && tiles[getTile(x + 1, y).id].physics == PHYS_NON_SOLID && rand() % 3 <= 1) 
	{
		setTile(x + 1, y, TILE_ROOT_R);
	}
}

void breakTree(int x, int y)
{
	Item woodStack;
	int freeSlot;
	int wood = 0;
	
	if(x > 0 && getTile(x - 1, y).id == TILE_ROOT_L)
	{
		setTile(x - 1, y, TILE_NOTHING);
		regionChange(x - 1, y);
		wood++;
	}
	if(x < game.WORLD_WIDTH - 1 && getTile(x + 1, y).id == TILE_ROOT_R)
	{
		setTile(x + 1, y, TILE_NOTHING);
		regionChange(x + 1, y);
		wood++;
	}
	for(; y >= 0 && (getTile(x, y).id == TILE_TRUNK || getTile(x, y).id == TILE_LEAVES); y--)
	{
		setTile(x, y, TILE_NOTHING);
		regionChange(x, y);
		wood++;
	}
	woodStack = (Item){ITEM_WOOD, PREFIX_NONE, wood << 1};
	while(woodStack.id != ITEM_NULL)
	{
		freeSlot = player.inventory.getFirstFreeSlot(woodStack.id);
		if(freeSlot > -1)
		{
			player.inventory.stackItem(&player.inventory.items[freeSlot], &woodStack);
		}
		else break;
	}
}

void generateCactus(int x, int y, int height)
{
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y - dY, TILE_CACTUS);
		setVar(x, y - dY);
		regionChange(x, y - dY);
		if(dY > 0 && rand() % 3 && getTile(x + 1, y - dY + 1).id == TILE_NOTHING)
		{
			setTile(x - 1, y - dY, TILE_CACTUS_BRANCH);
			setVar(x - 1, y - dY);
			regionChange(x - 1, y - dY);
		}
		else if(dY > 0 && rand() % 3 && getTile(x - 1, y - dY + 1).id == TILE_NOTHING)
		{
			setTile(x + 1, y - dY, TILE_CACTUS_BRANCH);
			setVar(x - 1, y - dY);
			regionChange(x + 1, y - dY);
		}
	}
}

void breakCactus(int x, int y)
{
	int freeSlot;

	while(getTile(x, y).id == TILE_CACTUS || getTile(x, y).id == TILE_CACTUS_BRANCH)
	{
		freeSlot = player.inventory.getFirstFreeSlot(ITEM_CACTUS);
		if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){ITEM_CACTUS, PREFIX_NONE, 1}));

		setTile(x, y, TILE_NOTHING);
		regionChange(x, y);

		if(getTile(x - 1, y).id == TILE_CACTUS_BRANCH)
		{
			freeSlot = player.inventory.getFirstFreeSlot(ITEM_CACTUS);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){ITEM_CACTUS, PREFIX_NONE, 1}));
			
			setTile(x - 1, y, TILE_NOTHING);
			regionChange(x - 1, y);
		}

		if(getTile(x + 1, y).id == TILE_CACTUS_BRANCH)
		{
			freeSlot = player.inventory.getFirstFreeSlot(ITEM_CACTUS);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){ITEM_CACTUS, PREFIX_NONE, 1}));
			
			setTile(x + 1, y, TILE_NOTHING);
			regionChange(x + 1, y);
		}

		y--;
	}
}

bool isSameOrFriend(int x, int y, unsigned char idx)
{
	Tile tile;
	const unsigned char *friends;

//	Outside world?
	if(x < 0 || x >= game.WORLD_WIDTH || y < 0 || y >= game.WORLD_HEIGHT) return 0;
	tile = getTile(x, y);
//	Same tile type?
	if(tile.id == idx) return 1;
	friends = tiles[idx].friends;
	for(int check = 0; check < MAX_FRIENDS; check++)
	{
//		Reached end of friends
		if(friends[check] == (unsigned char)-1) return 0;
//		This tile's type is a friend of the type of the tile we're checking
		if(tile.id == friends[check]) return 1;
	}
	return 0;
}

unsigned char findState(int x, int y)
{
	Tile tile = getTile(x, y);
	unsigned char sides = 0;

	sides |= isSameOrFriend(x - 1, y, tile.id);
	sides |= isSameOrFriend(x, y - 1, tile.id) << 1;
	sides |= isSameOrFriend(x + 1, y, tile.id) << 2;
	sides |= isSameOrFriend(x, y + 1, tile.id) << 3;

	return sides;
}

void regionChange(int x, int y)
{
	save.regionData[(y / REGION_SIZE) * save.regionsX + (x / REGION_SIZE)] = 1;
}

// Generic check for objects
bool checkArea(int x, int y, int width, int height, bool support)
{
	Tile tile;
	enum PhysicsTypes physics;

// 	Check the area is clear
	if(y < 0 || x < 0) return false;
	for(int dY = 0; dY < height; dY++)
	{
		if(y + dY >= game.WORLD_HEIGHT) return false;
		for(int dX = 0; dX < width; dX++)
		{
			if(x + dX >= game.WORLD_WIDTH) return false;
			tile = getTile(x + dX, y + dY);
			if(tile.id != TILE_NOTHING && tile.id != TILE_PLANT) return false;
		}
	}
	if(!support) return true;
// 	Check the tiles below can support the object
	if(y + height == game.WORLD_HEIGHT) return false;
	for(int dX = 0; dX < width; dX++)
	{
		tile = getTile(x + dX, y + height);
		physics = tiles[tile.id].physics;
		if(physics != PHYS_SOLID && physics != PHYS_SAND) return false;
	}
	return true;
}

// Specifically for 3-wide objects
bool place3Wide(int x, int y, int height, enum Tiles edge, enum Tiles middle, bool support)
{
	int xTemp;

	if(!checkArea(x, y, 3, height, support)) return false;
//	Place the middle
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x + 1, y + dY, middle);
		setVar(x + 1, y + dY);
	}
//	Place the edges
	for(int side = 0; side != 2; side++)
	{
		for(int dY = 0; dY < height; dY++)
		{
			xTemp = side ? x + 2 : x;
			setTile(xTemp, y + dY, edge);
			regionChange(xTemp, y + dY);
			setVar(xTemp, y + dY);
		}
	}
	return true;
}

// Does not have to be given the top-left tile
// Doesn't do bounds checking, make sure you are giving a valid object
void break3Wide(int x, int y, int height, enum Tiles edge, enum Tiles middle)
{
//	Find the top left tile
	if(getTile(x, y).id == edge)
	{
		if(getTile(x - 1, y).id == middle) x -= 2;
	}
	else x--;
	while(getTile(x, y - 1).id == edge) y--;

	for(int dY = 0; dY < height; dY++)
	{
		for(int dX = 0; dX < 3; dX++)
		{
			setTile(x + dX, y + dY, TILE_NOTHING);
			regionChange(x + dX, y + dY);
		}
	}
}

bool place2Wide(int x, int y, int height, enum Tiles left, enum Tiles right, bool support)
{
	if(!checkArea(x, y, 2, height, support)) return false;

	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, left);
		regionChange(x, y + dY);
		setVar(x, y + dY);
	}

	for(int dY = 0; dY < height; dY++)
	{
		setTile(x + 1, y + dY, right);
		regionChange(x + 1, y + dY);
		setVar(x + 1, y + dY);
	}

	return true;
}

// Doesn't do bounds checking, make sure you give a valid object
void break2Wide(int x, int y, int height, enum Tiles left)
{
	if(getTile(x, y).id != left) x--;
	while(getTile(x, y - 1).id == left) y--;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, TILE_NOTHING);
		regionChange(x, y + dY);
		setTile(x + 1, y + dY, TILE_NOTHING);
		regionChange(x + 1, y + dY);
	}
}

bool place1Wide(int x, int y, int height, enum Tiles tile, bool support)
{
	if(!checkArea(x, y, 1, height, support)) return false;

	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, tile);
		setVar(x, y + dY);
	}
	regionChange(x, y);

	return true;
}

void break1Wide(int x, int y, int height, enum Tiles tile)
{
	while(getTile(x, y - 1).id == tile) y--;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, TILE_NOTHING);
		regionChange(x, y + dY);
	}
}

void placeTile(int x, int y, Item *item)
{
	if(x < 0 || x >= game.WORLD_WIDTH || y < 0 || y >= game.WORLD_HEIGHT) return;
	Tile tile = getTile(x, y);
	bool success = true;
	int checkDeltas[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	int checkX, checkY;

	if(tile.id == TILE_NOTHING || tile.id == TILE_PLANT)
	{
		if(item->id != ITEM_NULL && items[item->id].tile > -1)
		{
			switch(item->id)
			{
				case ITEM_WBENCH:
					if(!place2Wide(x, y, 1, TILE_WBENCH_L, TILE_WBENCH_R, true)) success = false;
					break;
				
				case ITEM_ANVIL:
					if(!place2Wide(x, y, 1, TILE_ANVIL_L, TILE_ANVIL_R, true)) success = false;
					break;
				
				case ITEM_CRYST:
					if(!place2Wide(x, y, 2, TILE_CRYST_L, TILE_CRYST_R,	true)) success = false;
					break;
				
				case ITEM_CHEST:
					if(!checkArea(x, y, 2, 2, true) || !world.chests.addChest(x, y))
					{
						success = false;
						break;
					}
					if(!place2Wide(x, y, 2, TILE_CHEST_L, TILE_CHEST_R, true)) success = false;
					break;
				
				case ITEM_CHAIR:
					if(!place1Wide(x, y, 2, player.anim.direction ? TILE_CHAIR_R : TILE_CHAIR_L, true)) success = false;
					break;
				
				case ITEM_FURNACE:
					if(!place3Wide(x, y, 2, TILE_FURNACE_EDGE, TILE_FURNACE_MID, true)) success = false;
					break;
				
				case ITEM_DOOR:
					if(!place1Wide(x, y, 3, TILE_DOOR_C, true)) success = false;
					break;

				default:
					success = false;
					for(int check = 0; check < 4; check++)
					{
						checkX = x + checkDeltas[check][0];
						checkY = y + checkDeltas[check][1];
						if(checkX < 0 || checkX >= game.WORLD_WIDTH || checkY < 0 || checkY >= game.WORLD_HEIGHT) continue;
						if(getTile(checkX, checkY).id != TILE_NOTHING)
						{
							success = true;
							break;
						}
					}
					if(success)
					{
						setTile(x, y, items[item->id].tile);
						setVar(x, y);
					}
					break;
			}
			if(success)
			{
				updateMarkerChecks((Coords){x, y});
				regionChange(x, y);
				item->amount--;
				if(item->amount == 0) *item = (Item){ITEM_NULL, PREFIX_NONE, 0};
			}
		}
	}
}

void removeTile(int x, int y)
{
	if(x < 0 || x >= game.WORLD_WIDTH || y < 0 || y >= game.WORLD_HEIGHT) return;
	Tile tile = getTile(x, y);
	int freeSlot;

	regionChange(x,y);
	if(tile.id == TILE_TRUNK)
	{
		breakTree(x, y);
		return;
	}
	else if(tile.id == TILE_CACTUS || tile.id == TILE_CACTUS_BRANCH)
	{
		breakCactus(x, y);
		return;
	}
	if(getTile(x, y - 1).id == tile.id || tiles[getTile(x, y - 1).id].support != SUPPORT_KEEP)
	{
		if(tiles[tile.id].item != ITEM_NULL)
		{
			freeSlot = player.inventory.getFirstFreeSlot(tiles[tile.id].item);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){tiles[tile.id].item, PREFIX_NONE, 1}));
		}
		switch(tile.id)
		{
			case TILE_WBENCH_L: case TILE_WBENCH_R:
				break2Wide(x, y, 1, TILE_WBENCH_L);
				break;

			case TILE_ANVIL_L: case TILE_ANVIL_R:
				break2Wide(x, y, 1, TILE_ANVIL_L);
				break;
			
			case TILE_CRYST_L: case TILE_CRYST_R:
				break2Wide(x, y, 2, TILE_CRYST_L);
				break;
			
			case TILE_CHEST_R:
				x--;
			case TILE_CHEST_L:
				while(getTile(x, y - 1).id == TILE_CHEST_L) y--;
				world.chests.removeChest(x, y);
				break2Wide(x, y, 2, TILE_CHEST_L);
				break;

			case TILE_CHAIR_L: case TILE_CHAIR_R:
				break1Wide(x, y, 2, tile.id);
				break;
			
			case TILE_FURNACE_EDGE: case TILE_FURNACE_MID:
				break3Wide(x, y, 2, TILE_FURNACE_EDGE, TILE_FURNACE_MID);
				break;
			
			case TILE_DOOR_O_L_L: case TILE_DOOR_O_L_R:
				break2Wide(x, y, 3, TILE_DOOR_O_L_L);
				break;
			
			case TILE_DOOR_O_R_L: case TILE_DOOR_O_R_R:
				break2Wide(x, y, 3, TILE_DOOR_O_R_L);
				break;
			
			case TILE_DOOR_C:
				break1Wide(x, y, 3, TILE_DOOR_C);
				break;
			
			case TILE_GRASS:
				setTile(x, y, TILE_DIRT);
				break;

			default:
				setTile(x, y, TILE_NOTHING);
				break;
		}
		updateMarkerChecks((Coords){x, y});
		regionChange(x, y);

		tile = getTile(x, y);
		if(tiles[getTile(x, y - 1).id].support == SUPPORT_NEED) removeTile(x, y - 1);

		if(tiles[getTile(x, y + 1).id].support == SUPPORT_TOP) removeTile(x, y + 1);
	}
}

void openDoor(int x, int y)
{
	int direction;

	while(getTile(x, y - 1).id == TILE_DOOR_C) y--;
	if(!checkArea(x + 1, y, 1, 3, false) && !checkArea(x - 1, y, 1, 3, false)) return;
	direction = player.anim.direction;
	if(!checkArea(x + (direction ? -1 : 1), y, 1, 3, false)) direction ^= 1;
	break1Wide(x, y, 3, TILE_DOOR_C);
	if(direction)
	{
		place1Wide(x - 1, y, 3, TILE_DOOR_O_L_L, false);
		place1Wide(x, y, 3, TILE_DOOR_O_L_R, false);
	}
	else
	{
		place1Wide(x, y, 3, TILE_DOOR_O_R_L, false);
		place1Wide(x + 1, y, 3, TILE_DOOR_O_R_R, false);
	}
}

void closeDoor(int x, int y)
{
	switch(getTile(x, y).id)
	{
		case TILE_DOOR_O_L_L:
			x++;
		case TILE_DOOR_O_L_R:
			while(getTile(x, y - 1).id == TILE_DOOR_O_L_R) y--;
			break2Wide(x - 1, y, 3, TILE_DOOR_O_L_L);
			break;
		
		case TILE_DOOR_O_R_R:
			x--;
		case TILE_DOOR_O_R_L:
			while(getTile(x, y - 1).id == TILE_DOOR_O_R_L) y--;
			break2Wide(x, y, 3, TILE_DOOR_O_R_L);
			break;
	}
	place1Wide(x, y, 3, TILE_DOOR_C, false);
}

int spawnEntity(enum Entities entity, int x, int y)
{
	Entity *ent;

	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id == -1)
		{
			ent = &world.entities[idx];
			*ent = entityTemplates[entity];
			ent->props.x = x;
			ent->props.y = y;
			if(ent->init != NULL) ent->init(ent);

			return idx;
		}
	}

	return -1;
}

bool removeEntity(int idx)
{
	if(idx < 0 || idx >= MAX_ENTITIES) return false;
	if(world.entities[idx].id == -1) return false;
	world.entities[idx].id = -1;
	return true;
}

bool checkFreeEntitySlot()
{
	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id == -1) return true;
	}
	return false;
}

bool isDay()
{
	return world.timeTicks >= timeToTicks(4, 30) && world.timeTicks <= timeToTicks(19, 30);
}
