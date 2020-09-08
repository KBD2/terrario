#include <gint/std/stdlib.h>
#include <math.h>

#include "world.h"
#include "defs.h"
#include "save.h"

#define PI 3.14159265358979323846

extern bopti_image_t 
img_tile_nothing,
img_tile_stone,
img_tile_dirt,
img_tile_grass,
img_tile_wood,
img_tile_trunk,
img_tile_root_l,
img_tile_root_r,
img_tile_plant,
img_tile_wbench,
img_tile_platform,
img_tile_chair;

const TileData tiles[] = {
//      Ptr to sprite       Phys?			Render?	Type?			Support?		Friends (-1 to pad)							Item			Name
	{	&img_tile_nothing,	PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_NULL,		"Nothing"		},	// TILE_NOTHING
	{   &img_tile_stone,	PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, -1, -1},						ITEM_STONE,		"Stone"			},	// TILE_STONE
	{   &img_tile_dirt ,	PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_STONE, TILE_GRASS, -1},				ITEM_DIRT,		"Dirt"			},	// TILE_DIRT
	{	&img_tile_grass,	PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{TILE_DIRT, -1, -1},						ITEM_DIRT,		"Grass"			},	// TILE_GRASS
	{	&img_tile_wood,		PHYS_SOLID,		true,	TYPE_SHEET_VAR,	SUPPORT_NONE,	{-1, -1, -1},								ITEM_WOOD,		"Wood"			},	// TILE_WOOD
	{	&img_tile_trunk,	PHYS_NON_SOLID,	true,	TYPE_SHEET_VAR,	SUPPORT_KEEP,	{TILE_ROOT_L, TILE_ROOT_R, TILE_LEAVES},	ITEM_WOOD,		"Tree Trunk"	},	// TILE_TRUNK
	{	&img_tile_root_l,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1, -1, -1},								ITEM_WOOD,		"Tree Root"		},	// TILE_ROOT_L
	{	&img_tile_root_r,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_KEEP,	{-1, -1, -1},								ITEM_WOOD,		"Tree Root"		},	// TILE_ROOT_R
	{	&img_tile_nothing,	PHYS_NON_SOLID,	false,	TYPE_TILE,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_WOOD,		"TreeTop"		},	// TILE_LEAVES
	{	&img_tile_plant,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_NULL,		"Plant"			},	// TILE_PLANT
	{	&img_tile_wbench,	PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_WBENCH,	"Workbench L"	},	// TILE_WBENCH_L
	{	&img_tile_wbench,	PHYS_PLATFORM,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_WBENCH,	"Workbench R"	},	// TILE_WBENCH_R
	{	&img_tile_platform,	PHYS_PLATFORM,	true,	TYPE_SHEET,		SUPPORT_NONE,	{-1, -1, -1},								ITEM_PLATFORM,	"Platform"		},	// TILE_PLATFORM
	{	&img_tile_chair,	PHYS_NON_SOLID,	true,	TYPE_TILE_VAR,	SUPPORT_NEED,	{-1, -1, -1},								ITEM_CHAIR,		"Chair"			}	// TILE_CHAIR
};

unsigned char makeVar()
{
	return (unsigned int)rand() % 3;
}

float interpolate(float a, float b, float x){
	float f = (1.0 - cosf(x * PI)) * 0.5;
    return a * (1.0 - f) + b * f;
}

float randFloat()
{
	return (float)rand() / __RAND_MAX;
}

void generateTree(int x, int y)
{
	unsigned int top = 0;
	unsigned int height = 2 + (unsigned int)rand() % 5;

	for(unsigned int i = 0; i < height; i++) if(
		getTile(x - 1, y - i).idx == TILE_TRUNK
	 || getTile(x + 1, y - i).idx == TILE_TRUNK
	 || getTile(x - 2, y - i).idx == TILE_TRUNK
	 || getTile(x + 2, y - i).idx == TILE_TRUNK
	 ) return;

	for(; top < height; top++)
	{
		getTile(x, y - top) = (Tile){TILE_TRUNK, makeVar()};
	}
	getTile(x, y - top) = (Tile){TILE_LEAVES, makeVar()};
	if(getTile(x - 1, y + 1).idx == TILE_GRASS && tiles[getTile(x - 1, y).idx].physics == PHYS_NON_SOLID && (unsigned int)rand() % 3 <= 1)
	{
		getTile(x - 1, y) = (Tile){TILE_ROOT_L, makeVar()};
	} 
	if(getTile(x + 1, y + 1).idx == TILE_GRASS && tiles[getTile(x + 1, y).idx].physics == PHYS_NON_SOLID && (unsigned int)rand() % 3 <= 1) 
	{
		getTile(x + 1, y) = (Tile){TILE_ROOT_R, makeVar()};
	}
}

void breakTree(int x, int y)
{
	Item woodStack;
	int freeSlot;
	int wood = 0;
	
	if(getTile(x - 1, y).idx == TILE_ROOT_L)
	{
		getTile(x - 1, y) = (Tile){TILE_NOTHING, 0};
		regionChange(x - 1, y);
		wood++;
	}
	if(getTile(x + 1, y).idx == TILE_ROOT_R)
	{
		getTile(x + 1, y) = (Tile){TILE_NOTHING, 0};
		regionChange(x + 1, y);
		wood++;
	}
	for(; getTile(x, y).idx == TILE_TRUNK || getTile(x, y).idx == TILE_LEAVES; y--)
	{
		getTile(x, y) = (Tile){TILE_NOTHING, 0};
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

/*
Top third of world: Air
1/6th of world: Dirt
Bottom half of world: Stone
*/
void generateWorld()
{
	float amplitude = 10;
	int wavelength = 20;
	float a, b;
	int perlinY;

//	Make some basic layers
	for(unsigned int y = 0; y < WORLD_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < WORLD_WIDTH; x++)
		{
			if(y >= WORLD_HEIGHT / 2)
			{
				getTile(x, y) = (Tile){TILE_STONE, makeVar()};
			} else if(y >= WORLD_HEIGHT / 2 - WORLD_HEIGHT / 12)
			{
				getTile(x, y) = (Tile){TILE_DIRT, makeVar()};
			} else
			{
				getTile(x, y) = (Tile){TILE_NOTHING, 0};
			}
		}
	}

//	Make some hills using Perlin noise
	a = randFloat();
	b = randFloat();
	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		if(x % wavelength == 0){
			a = b;
			b = randFloat();
			perlinY = WORLD_HEIGHT / 3 + a * amplitude;
		}
		else
		{
			perlinY = WORLD_HEIGHT / 3 + interpolate(a, b, (float)(x % wavelength) / wavelength) * amplitude;
		}
		getTile(x, perlinY) = (Tile){TILE_GRASS, makeVar()};
		for(int hillY = perlinY + 1; hillY < WORLD_HEIGHT / 2; hillY++) getTile(x, hillY) = (Tile){TILE_DIRT, makeVar()};
	}

	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 0; y < WORLD_HEIGHT; y++)
		{
			if(getTile(x, y).idx == TILE_GRASS)
			{
				if(rand() % 8 == 0) generateTree(x, y - 1);
				else if(rand() % 2 == 0) getTile(x, y - 1) = (Tile){TILE_PLANT, makeVar()};
				if(getTile(x + 1, y).idx == TILE_NOTHING || getTile(x - 1, y).idx == TILE_NOTHING)
				{
					getTile(x, y + 1) = (Tile){TILE_GRASS, makeVar()};
				}
				break;
			}
		}
	}
}

bool isSameOrFriend(int x, int y, unsigned char idx)
{
	Tile *tile;
	const unsigned char *friends;

//	Outside world?
	if(x < 0 || x >= WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT) return 0;
	tile = &getTile(x, y);
//	Same tile type?
	if(tile->idx == idx) return 1;
	friends = tiles[idx].friends;
	for(int check = 0; check < MAX_FRIENDS; check++)
	{
//		This tile's type is a friend of the type of the tile we're checking
		if(tile->idx == friends[check]) return 1;
	}
	return 0;
}

unsigned char findState(int x, int y)
{
	Tile *tile = &getTile(x, y);
	unsigned char sides = 0;

	sides |= isSameOrFriend(x - 1, y, tile->idx);
	sides |= isSameOrFriend(x, y - 1, tile->idx) << 1;
	sides |= isSameOrFriend(x + 1, y, tile->idx) << 2;
	sides |= isSameOrFriend(x, y + 1, tile->idx) << 3;

	return sides;
}

void regionChange(int x, int y)
{
	save.regionData[(y / REGION_SIZE) * save.regionsX + (x / REGION_SIZE)] = 1;
}

bool checkCanOverwrite(int x, int y)
{
	Tile *tile = &getTile(x, y);
	return tile->idx == TILE_NOTHING || tile->idx == TILE_PLANT;
}

bool checkCanSupport(int x, int y)
{
	Tile *tile = &getTile(x, y);
	return tile->idx == TILE_PLATFORM || tiles[tile->idx].physics == PHYS_SOLID;
}

void placeTile(int x, int y, Item *item)
{
	Tile *tile = &getTile(x, y);
	bool success = true;

	if(tile->idx == TILE_NOTHING || tile->idx == TILE_PLANT)
	{
		if(item->id != ITEM_NULL && items[item->id].tile > -1)
		{
			if(tiles[items[item->id].tile].support == SUPPORT_NEED && !checkCanSupport(x, y + 1)) return;
			switch(item->id)
			{
				case ITEM_WBENCH:
					if(!checkCanSupport(x + 1, y + 1) || !checkCanOverwrite(x + 1, y) || x == WORLD_WIDTH - 1)
					{
						success = false;
						break;
					}
					*tile = (Tile){TILE_WBENCH_L, 0};
					getTile(x + 1, y) = (Tile){TILE_WBENCH_R, 1};
					regionChange(x + 1, y);
					break;
				
				case ITEM_CHAIR:
					if(!checkCanOverwrite(x, y - 1) || y == WORLD_HEIGHT - 1)
					{
						success = false;
						break;
					}
					if(player.anim.direction)
					{
						*tile = (Tile){TILE_CHAIR, 0};
						getTile(x, y - 1) = (Tile){TILE_CHAIR, 1};
						regionChange(x, y - 1);
					}
					else
					{
						*tile = (Tile){TILE_CHAIR, 2};
						getTile(x, y - 1) = (Tile){TILE_CHAIR, 3};
						regionChange(x, y - 1);
					}
					break;

				default:
					*tile = (Tile){items[item->id].tile, makeVar()};
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
	Tile *tile = &getTile(x, y);
	int freeSlot;
	const Tile nothing = {TILE_NOTHING, 0};

	regionChange(x,y);
	if(tile->idx == TILE_TRUNK)
	{
		breakTree(x, y);
		return;
	}
	if(tiles[getTile(x, y - 1).idx].support != SUPPORT_KEEP)
	{
		if(tiles[tile->idx].item != ITEM_NULL)
		{
			freeSlot = player.inventory.getFirstFreeSlot(tiles[tile->idx].item);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){tiles[tile->idx].item, 1}));
		}
		switch(tile->idx)
		{
			case TILE_WBENCH_L:
				*tile = nothing;
				getTile(x + 1, y) = nothing;
				regionChange(x + 1, y);
				break;
			case TILE_WBENCH_R:
				*tile = nothing;
				getTile(x - 1, y) = nothing;
				regionChange(x - 1, y);
				break;

			case TILE_CHAIR:
				*tile = nothing;
				if(getTile(x, y - 1).idx == TILE_NOTHING)
				{
					getTile(x, y + 1) = nothing;
					regionChange(x, y + 1);
				}
				else
				{
					getTile(x, y - 1) = nothing;
					regionChange(x, y - 1);
				}
				break;
			
			case TILE_GRASS:
				tile->idx = TILE_DIRT;
				break;

			default:
				*tile = nothing;
				break;
		}
		regionChange(x, y);
		if(tile->idx == TILE_NOTHING && tiles[getTile(x, y - 1).idx].support == SUPPORT_NEED) removeTile(x, y - 1);
	}
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
			ent->init(ent);

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