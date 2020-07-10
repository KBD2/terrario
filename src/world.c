#include <stdlib.h>
#include <math.h>

#include <gint/display.h>

#include "world.h"
#include "defs.h"
#include "syscalls.h"

#define PI 3.14159265358979323846

extern image_t 
img_tile_nothing,
img_tile_stone,
img_tile_dirt,
img_tile_grass;

const TileData tiles[] = {
//      Ptr to sprite       	Solid?		Render?		Sheet?		Friends (-1 to pad)
	{	&img_tile_nothing	,	false	,	false	,	false	,	{-1, -1}					},
	{   &img_tile_stone		,	true	,	true	,	true	,	{TILE_DIRT, -1}				},
	{   &img_tile_dirt 		,	true	,	true	,	true	,	{TILE_STONE, TILE_GRASS}	},
	{	&img_tile_grass		,	true	,	true	,	true	,	{TILE_DIRT, -1}				}
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

void generateWorld(struct World* world)
{
	float amplitude = 10;
	int wavelength = 20;
	float a, b;
	int perlinY;

	world->tiles = malloc(WORLD_WIDTH * WORLD_HEIGHT * sizeof(Tile));

//	Make some basic layers
	for(unsigned int y = 0; y < WORLD_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < WORLD_WIDTH; x++)
		{
			if(y >= WORLD_HEIGHT / 2)
			{
				world->tiles[y * WORLD_WIDTH + x] = (Tile){TILE_STONE, 0};
			} else if(y >= WORLD_HEIGHT / 2 - WORLD_HEIGHT / 12)
			{
				world->tiles[y * WORLD_WIDTH + x] = (Tile){TILE_DIRT, 0};
			} else
			{
				world->tiles[y * WORLD_WIDTH + x] = (Tile){TILE_NOTHING, 0};
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
		world->tiles[perlinY * WORLD_WIDTH + x] = (Tile){TILE_GRASS, 0};
		for(perlinY++; perlinY < WORLD_HEIGHT / 2; perlinY++) world->tiles[perlinY * WORLD_WIDTH + x] = (Tile){TILE_DIRT, 0};
	}

	for(int y = 0; y < WORLD_HEIGHT; y++)
	{
		for(int x = 0; x < WORLD_WIDTH; x++)
		{
			updateStates(world, x, y);
		}
	}
}

bool isSameOrFriend(struct World* world, int x, int y, unsigned char idx)
{
	Tile* tile;
	const unsigned char* friends;

//	Outside world?
	if(x < 0 || x >= WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT) return 0;
	tile = &world->tiles[y * WORLD_WIDTH + x];
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

void findState(struct World* world, int x, int y)
{
	if(x < 0 || x >= WORLD_WIDTH || y < 0 || y > WORLD_HEIGHT) return;
	Tile* tile = &world->tiles[y * WORLD_WIDTH + x];
	unsigned char sides = 0;

	sides |= isSameOrFriend(world, x - 1, y, tile->idx);
	sides |= isSameOrFriend(world, x, y - 1, tile->idx) << 1;
	sides |= isSameOrFriend(world, x + 1, y, tile->idx) << 2;
	sides |= isSameOrFriend(world, x, y + 1, tile->idx) << 3;

	tile->state = sides;
}

void updateStates(struct World* world, int x, int y)
{
	for(int dY = -1; dY < 2; dY++)
	{
		for(int dX = -1; dX < 2; dX++)
		{
			findState(world, x + dX, y + dY);
		}
	}
}