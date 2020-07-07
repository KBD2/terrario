#include <gint/std/stdlib.h>
#include <gint/std/stdio.h>
#include <gint/gray.h>
#include <gint/display.h>

#include "map.h"
#include "defs.h"

#define PI 3.14159
#define TERMS 7
#define RAND_MAX 0x7FFFFFFF

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

float pow(float base, int exp) {
    if(exp < 0) {
        if(base == 0)
            return -0;
        return 1.0 / (base * pow(base, (-exp) - 1));
    }
    if(exp == 0)
        return 1;
    if(exp == 1)
        return base;
    return base * pow(base, exp - 1);
}

int fact(int n) {
    return n <= 0 ? 1 : n * fact(n-1);
}

float cos(int deg) {
    deg %= 360;
    float rad = (float)deg * PI / 180;
    float cos = 0;

    int i;
    for(i = 0; i < TERMS; i++)
	{
        cos += pow(-1, i) * pow(rad, 2 * i) / fact(2 * i);
    }
    return cos;
}

float interpolate(float a, float b, float x){
	float f = (1.0 - cos(x * 180)) * 0.5;
    return a * (1.0 - f) + b * f;
}

void generateMap(struct Map* map)
{
	float amplitude = 5;
	int wavelength = 10;
	float a, b;
	int perlinY;

	map->tiles = malloc(MAP_WIDTH * MAP_HEIGHT);

//	Make some basic layers
	for(unsigned int y = 0; y < MAP_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < MAP_WIDTH; x++)
		{
			if(y >= 40)
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_STONE];
			} else if(y >= 30)
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_DIRT];
			} else
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_NOTHING];
			}
		}
	}

//	Make some hills using Perlin noise
	a = (float)(rand() % 10000) / 10000;
	b = (float)(rand() % 10000) / 10000;
	for(int x = 0; x < MAP_WIDTH; x++)
	{
		if(x % (int)wavelength == 0){
			a = b;
			b = (float)(rand() % 10000) / 10000;
			perlinY = MAP_HEIGHT / 2 + a * amplitude;
		}
		else
		{
			perlinY = MAP_HEIGHT / 2 + interpolate(a, b, (float)(x % wavelength) / wavelength) * amplitude;
		}
		for(; perlinY < 30; perlinY++) map->tiles[perlinY * MAP_WIDTH + x] = &tiles[TILE_DIRT];
	}
}