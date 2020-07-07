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
	map->tiles = malloc(MAP_WIDTH * MAP_HEIGHT);
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

	int amp = 5;
	int wl = 10;
	float a = (float)(rand() % 10000) / 10000;
	float b = (float)(rand() % 10000) / 10000;
	int x = 0;
	int y = MAP_HEIGHT / 2;
	while(x < MAP_WIDTH)
	{
		if(x % wl == 0){
			a = b;
			b = (float)(rand() % 10000) / 10000;
			y = MAP_HEIGHT / 2 + a * amp;
		}
		else
		{
			y = MAP_HEIGHT / 2 + interpolate(a, b, (float)(x % wl) / wl) * (float)amp;
		}
		for(int y2 = y; y2 < 30; y2++) map->tiles[y2 * MAP_WIDTH + x] = &tiles[TILE_DIRT];
		x += 1;
	}
}