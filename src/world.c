#include <stdlib.h>
#include <math.h>

#include "world.h"
#include "defs.h"
#include "save.h"

#define PI 3.14159265358979323846

extern bopti_image_t 
img_tile_nothing,
img_tile_stone,
img_tile_dirt,
img_tile_grass;

const TileData tiles[] = {
//      Ptr to sprite       	Solid?		Render?		Sheet?		Friends (-1 to pad)
	{	&img_tile_nothing	,	false	,	false	,	false	,	{-1, -1}					,	ITEM_NULL	},	// TILE_NOTHING
	{   &img_tile_stone		,	true	,	true	,	true	,	{TILE_DIRT, -1}				,	ITEM_STONE	},	// TILE_STONE
	{   &img_tile_dirt 		,	true	,	true	,	true	,	{TILE_STONE, TILE_GRASS}	,	ITEM_DIRT	},	// TILE_DIRT
	{	&img_tile_grass		,	true	,	true	,	true	,	{TILE_DIRT, -1}				,	ITEM_DIRT	}	// TILE_GRASS
};

float interpolate(float a, float b, float x){
	float f = (1.0 - cosf(x * PI)) * 0.5;
    return a * (1.0 - f) + b * f;
}

float randFloat()
{
	return (float)rand() / RAND_MAX;
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
				getTile(x, y) = (Tile){TILE_STONE};
			} else if(y >= WORLD_HEIGHT / 2 - WORLD_HEIGHT / 12)
			{
				getTile(x, y) = (Tile){TILE_DIRT};
			} else
			{
				getTile(x, y) = (Tile){TILE_NOTHING};
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
		getTile(x, perlinY) = (Tile){TILE_GRASS};
		for(int hillY = perlinY + 1; hillY < WORLD_HEIGHT / 2; hillY++) getTile(x, hillY) = (Tile){TILE_DIRT};
	}

	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 0; y < WORLD_HEIGHT; y++)
		{
			if(getTile(x, y).idx == TILE_GRASS)
			{
				if(getTile(x + 1, y).idx == TILE_NOTHING || getTile(x - 1, y).idx == TILE_NOTHING)
				{
					getTile(x, y + 1) = (Tile){TILE_GRASS};
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