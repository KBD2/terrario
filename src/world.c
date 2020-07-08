#include <stdlib.h>
#include <math.h>

#include "world.h"
#include "defs.h"

#define PI 3.14159265358979323846

extern image_t 
img_tile_nothing,
img_tile_stone,
img_tile_dirt;

const Tile tiles[] = {
//      Ptr to sprite       	Solid?	Render?
	{	&img_tile_nothing	,	false,	false	},
	{   &img_tile_stone		,	true,	true	},
	{   &img_tile_dirt 		,	true,	true	}
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

	world->tiles = malloc(WORLD_WIDTH * WORLD_HEIGHT * sizeof(unsigned char));

//	Make some basic layers
	for(unsigned int y = 0; y < WORLD_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < WORLD_WIDTH; x++)
		{
			if(y >= WORLD_HEIGHT / 2)
			{
				world->tiles[y * WORLD_WIDTH + x] = TILE_STONE;
			} else if(y >= WORLD_HEIGHT / 2 - WORLD_HEIGHT / 12)
			{
				world->tiles[y * WORLD_WIDTH + x] = TILE_DIRT;
			} else
			{
				world->tiles[y * WORLD_WIDTH + x] = TILE_NOTHING;
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
		for(; perlinY < WORLD_HEIGHT / 2; perlinY++) world->tiles[perlinY * WORLD_WIDTH + x] = TILE_DIRT;
	}
}