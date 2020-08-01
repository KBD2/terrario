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
img_tile_root_r;

const TileData tiles[] = {
//      Ptr to sprite       	Solid?		Render?		Type?			Support?	Friends (-1 to pad)							Item			Name
	{	&img_tile_nothing	,	false	,	false	,	TILE		,	false	,	{-1, -1, -1}							,	ITEM_NULL	,	"Nothing"	},	// TILE_NOTHING
	{   &img_tile_stone		,	true	,	true	,	SHEET_VAR	,	false	,	{TILE_DIRT, -1, -1}						,	ITEM_STONE	,	"Stone"		},	// TILE_STONE
	{   &img_tile_dirt 		,	true	,	true	,	SHEET_VAR	,	false	,	{TILE_STONE, TILE_GRASS, -1}			,	ITEM_DIRT	,	"Dirt"		},	// TILE_DIRT
	{	&img_tile_grass		,	true	,	true	,	SHEET_VAR	,	false	,	{TILE_DIRT, -1, -1}						,	ITEM_DIRT	,	"Grass"		},	// TILE_GRASS
	{	&img_tile_wood		,	true	,	true	,	SHEET_VAR	,	false	,	{-1, -1, -1}							,	ITEM_WOOD	,	"Wood"		},	// TILE_WOOD
	{	&img_tile_trunk		,	false	,	true	,	SHEET_VAR	,	true	,	{TILE_ROOT_L, TILE_ROOT_R, TILE_LEAVES}	,	ITEM_WOOD	,	"Tree Trunk"},	// TILE_TRUNK
	{	&img_tile_root_l	,	false	,	true	,	TILE_VAR	,	true	,	{-1, -1, -1}							,	ITEM_WOOD	,	"Tree Root"	},	// TILE_ROOT_L
	{	&img_tile_root_r	,	false	,	true	,	TILE_VAR	,	true	,	{-1, -1, -1}							,	ITEM_WOOD	,	"Tree Root"	},	// TILE_ROOT_R
	{	&img_tile_nothing	,	false	,	false	,	TILE		,	false	,	{-1, -1, -1}							,	ITEM_WOOD	,	"Leaves"	}	// TILE_LEAVES
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
	getTile(x, y - top) = (Tile){TILE_LEAVES, 0};
	if(getTile(x - 1, y + 1).idx != TILE_NOTHING && (unsigned int)rand() % 2 == 0) getTile(x - 1, y) = (Tile){TILE_ROOT_L, makeVar()};
	if(getTile(x + 1, y + 1).idx != TILE_NOTHING && (unsigned int)rand() % 2 == 0) getTile(x + 1, y) = (Tile){TILE_ROOT_R, makeVar()};
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
	woodStack = (Item){ITEM_WOOD, wood};
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
				if(rand() % 10 == 0) generateTree(x, y - 1);
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
	Tile* tile;
	const unsigned char* friends;

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
	Tile* tile = &getTile(x, y);
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