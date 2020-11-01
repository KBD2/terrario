#include <gint/std/stdlib.h>
#include <gint/defs/util.h>
#include <math.h>
#include <string.h>

#include "world.h"
#include "defs.h"
#include "save.h"

#include <gint/gray.h>

#define PI 3.14159265358979323846

extern bopti_image_t 
img_tile_nothing,
img_tile_stone,
img_tile_dirt,
img_tile_grass,
img_tile_wood,
img_tile_trunk,
img_tile_root_l, img_tile_root_r,
img_tile_plant,
img_tile_wbench,
img_tile_platform,
img_tile_chair,
img_tile_torch,
img_tile_furnace_edge, img_tile_furnace_mid,
img_tile_iron_ore,
img_tile_anvil,
img_tile_chest,
img_tile_door_c,
img_tile_door_o_l_l, img_tile_door_o_l_r,
img_tile_door_o_r_l, img_tile_door_o_r_r,
img_tile_vine;

const TileData tiles[] = {
//      Ptr to sprite       	Phys?			Render?	Type?			Support?		Friends (-1 to pad)							Item			Name
	{	&img_tile_nothing,		PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_NULL,		"Nothing"		},	// TILE_NOTHING
	{   &img_tile_stone,		PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_IRON_ORE, -1},				ITEM_STONE,		"Stone"			},	// TILE_STONE
	{   &img_tile_dirt ,		PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_STONE, TILE_GRASS, TILE_IRON_ORE},	ITEM_DIRT,		"Dirt"			},	// TILE_DIRT
	{	&img_tile_grass,		PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, -1, -1},						ITEM_NULL,		"Grass"			},	// TILE_GRASS
	{	&img_tile_wood,			PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{-1, -1, -1},								ITEM_WOOD,		"Wood"			},	// TILE_WOOD
	{	&img_tile_trunk,		PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_KEEP,	{TILE_ROOT_L, TILE_ROOT_R, TILE_LEAVES},	ITEM_WOOD,		"Tree Trunk"	},	// TILE_TRUNK
	{	&img_tile_root_l,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1, -1, -1},								ITEM_WOOD,		"Tree Root"		},	// TILE_ROOT_L
	{	&img_tile_root_r,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1, -1, -1},								ITEM_WOOD,		"Tree Root"		},	// TILE_ROOT_R
	{	&img_tile_nothing,		PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_WOOD,		"TreeTop"		},	// TILE_LEAVES
	{	&img_tile_plant,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_NULL,		"Plant"			},	// TILE_PLANT
	{	&img_tile_wbench,		PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_WBENCH,	"Workbench L"	},	// TILE_WBENCH_L
	{	&img_tile_wbench,		PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_WBENCH,	"Workbench R"	},	// TILE_WBENCH_R
	{	&img_tile_platform,		PHYS_PLATFORM,	true,	TYPE_SHEET,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_PLATFORM,	"Platform"		},	// TILE_PLATFORM
	{	&img_tile_chair,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_CHAIR,		"Chair"			},	// TILE_CHAIR
	{	&img_tile_torch,		PHYS_NON_SOLID,	true,	TYPE_SHEET,		SUPPORT_NONE,	{TILE_NOTHING, -1, -1},						ITEM_TORCH,		"Torch"			},	// TILE_TORCH
	{	&img_tile_furnace_edge,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_FURNACE,	"Furnace"		},	// TILE_FURNACE_EDGE
	{	&img_tile_furnace_mid,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_FURNACE,	"Furnace"		},	// TILE_FURNACE_MID
	{	&img_tile_iron_ore,		PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, TILE_STONE, -1},				ITEM_IRON_ORE,	"Iron Ore"		},	// TILE_IRON_ORE
	{	&img_tile_anvil,		PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_ANVIL,		"Anvil L"		},	// TILE_ANVIL_L
	{	&img_tile_anvil,		PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_ANVIL,		"Anvil R"		},	// TILE_ANVIL_R
	{	&img_tile_chest,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_CHEST,		"Chest L"		},	// TILE_CHEST_L
	{	&img_tile_chest,		PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_CHEST,		"Chest R"		},	// TILE_CHEST_R
	{	&img_tile_door_c,		PHYS_SOLID,		true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_DOOR,		"Door C"		},	// TILE_DOOR_C
	{	&img_tile_door_o_l_l,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NONE,	{-1, -1, -1},								ITEM_DOOR,		"Door O L L",	},	// TILE_DOOR_O_L_L
	{	&img_tile_door_o_l_r,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_DOOR,		"Door O L R",	},	// TILE_DOOR_O_L_R
	{	&img_tile_door_o_r_l,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_DOOR,		"Door O R L",	},	// TILE_DOOR_O_R_L
	{	&img_tile_door_o_r_r,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NONE,	{-1, -1, -1},								ITEM_DOOR,		"Door O R R",	},	// TILE_DOOR_O_R_R
	{	&img_tile_vine,			PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_TOP,	{-1, -1, -1},								ITEM_NULL,		"Vine",			},	// TILE_VINE
};

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
		setTile(x, y - top, TILE_TRUNK, makeVar());
	}
	setTile(x, y - top, TILE_LEAVES, makeVar());
	if(getTile(x - 1, y + 1).id == TILE_GRASS && tiles[getTile(x - 1, y).id].physics == PHYS_NON_SOLID && rand() % 3 <= 1)
	{
		setTile(x - 1, y, TILE_ROOT_L, makeVar());
	} 
	if(getTile(x + 1, y + 1).id == TILE_GRASS && tiles[getTile(x + 1, y).id].physics == PHYS_NON_SOLID && rand() % 3 <= 1) 
	{
		setTile(x + 1, y, TILE_ROOT_R, makeVar());
	}
}

void breakTree(int x, int y)
{
	Item woodStack;
	int freeSlot;
	int wood = 0;
	
	if(x > 0 && getTile(x - 1, y).id == TILE_ROOT_L)
	{
		setTile(x - 1, y, TILE_NOTHING, 0);
		regionChange(x - 1, y);
		wood++;
	}
	if(x < game.WORLD_WIDTH - 1 && getTile(x + 1, y).id == TILE_ROOT_R)
	{
		setTile(x + 1, y, TILE_NOTHING, makeVar());
		regionChange(x + 1, y);
		wood++;
	}
	for(; y >= 0 && (getTile(x, y).id == TILE_TRUNK || getTile(x, y).id == TILE_LEAVES); y--)
	{
		setTile(x, y, TILE_NOTHING, 0);
		regionChange(x, y);
		wood++;
	}
	woodStack = (Item){ITEM_WOOD, wood << 1};
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
		if(tiles[tile.id].physics != PHYS_SOLID) return false;
	}
	return true;
}

// Specifically for 3-wide objects
bool place3Wide(int x, int y, int height, enum Tiles edge, enum Tiles middle, bool support)
{
	int xTemp;
	int variation = 0;

//	Place the edges
	if(!checkArea(x, y, 3, height, support)) return false;
	for(int side = 0; side != 2; side++)
	{
		for(int dY = 0; dY < height; dY++)
		{
			xTemp = side ? x + 2 : x;
			setTile(xTemp, y + dY, edge, variation);
			regionChange(xTemp, y + dY);
			variation++;
		}
	}
//	Place the middle
	variation = 0;
	x++;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, middle, variation);
		variation++;
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
	while(getTile(x, y).variant != 0) y--;

	for(int dY = 0; dY < height; dY++)
	{
		for(int dX = 0; dX < 3; dX++)
		{
			setTile(x + dX, y + dY, TILE_NOTHING, 0);
			regionChange(x + dX, y + dY);
		}
	}
}

bool place2Wide(int x, int y, int height, enum Tiles left, enum Tiles right, bool support)
{
	int var = 0;

	if(!checkArea(x, y, 2, height, support)) return false;

	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y, left, var);
		regionChange(x, y + dY);
		var++;
	}

	for(int dY = 0; dY < height; dY++)
	{
		setTile(x + 1, y + dY, right, var);
		regionChange(x + 1, y + dY);
		var++;
	}

	return true;
}

// Doesn't do bounds checking, make sure you give a valid object
void break2Wide(int x, int y, int height, enum Tiles left)
{
	if(getTile(x, y).id != left) x--;
	while(getTile(x, y).variant > 0) y--;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, TILE_NOTHING, 0);
		regionChange(x, y + dY);
		setTile(x + 1, y + dY, TILE_NOTHING, 0);
		regionChange(x + 1, y + dY);
	}
}

bool place1Wide(int x, int y, int height, enum Tiles tile, int startVar, bool support)
{
	int var = 0;

	if(!checkArea(x, y, 1, height, support)) return false;

	if(startVar > 0) var = startVar;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, tile, var);
		var++;
	}
	regionChange(x, y);

	return true;
}

void break1Wide(int x, int y, int height, enum Tiles tile)
{
	while(getTile(x, y - 1).id == tile) y--;
	for(int dY = 0; dY < height; dY++)
	{
		setTile(x, y + dY, TILE_NOTHING, 0);
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
				
				case ITEM_CHEST:
					if(!checkArea(x, y, 2, 2, true) || !world.chests.addChest(x, y))
					{
						success = false;
						break;
					}
					if(!place2Wide(x, y, 2, TILE_CHEST_L, TILE_CHEST_R, true)) success = false;
					break;
				
				case ITEM_CHAIR:
					if(!place1Wide(x, y, 2, TILE_CHAIR, player.anim.direction ? 0 : 2, true)) success = false;
					break;
				
				case ITEM_FURNACE:
					if(!place3Wide(x, y, 2, TILE_FURNACE_EDGE, TILE_FURNACE_MID, true)) success = false;
					break;
				
				case ITEM_DOOR:
					if(!place1Wide(x, y, 3, TILE_DOOR_C, -1, true)) success = false;
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
					if(success) setTile(x, y, items[item->id].tile, makeVar());
					break;
			}
			if(success)
			{
				regionChange(x, y);
				item->amount--;
				if(item->amount == 0) *item = (Item){ITEM_NULL, 0};
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
	if(tiles[getTile(x, y - 1).id].support != SUPPORT_KEEP)
	{
		if(tiles[tile.id].item != ITEM_NULL)
		{
			freeSlot = player.inventory.getFirstFreeSlot(tiles[tile.id].item);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){tiles[tile.id].item, 1}));
		}
		switch(tile.id)
		{
			case TILE_WBENCH_L: case TILE_WBENCH_R:
				break2Wide(x, y, 1, TILE_WBENCH_L);
				break;

			case TILE_ANVIL_L: case TILE_ANVIL_R:
				break2Wide(x, y, 1, TILE_ANVIL_L);
				break;
			
			case TILE_CHEST_R:
				x--;
			case TILE_CHEST_L:
				while(getTile(x, y).variant != 0) y--;
				world.chests.removeChest(x, y);
				break2Wide(x, y, 2, TILE_CHEST_L);
				break;

			case TILE_CHAIR:
				break1Wide(x, y, 2, TILE_CHAIR);
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
				setTile(x, y, TILE_DIRT, makeVar());
				break;

			default:
				setTile(x, y, TILE_NOTHING, 0);
				break;
		}
		regionChange(x, y);

		tile = getTile(x, y);
		if(tile.id == TILE_NOTHING && tiles[getTile(x, y - 1).id].support == SUPPORT_NEED) removeTile(x, y - 1);

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
		place1Wide(x - 1, y, 3, TILE_DOOR_O_L_L, -1, false);
		place1Wide(x, y, 3, TILE_DOOR_O_L_R, -1, false);
	}
	else
	{
		place1Wide(x, y, 3, TILE_DOOR_O_R_L, -1, false);
		place1Wide(x + 1, y, 3, TILE_DOOR_O_R_R, -1, false);
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
	place1Wide(x, y, 3, TILE_DOOR_C, -1, false);
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