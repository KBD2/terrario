#include "map.h"
#include "defs.h"

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

void generateMap(struct Map* map)
{
	map->tiles = malloc(MAP_WIDTH * MAP_HEIGHT);
//	TODO: Research ways to procedurally generate worlds
	for(unsigned int y = 0; y < MAP_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < MAP_WIDTH; x++)
		{
			if(y >= 30)
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_STONE];
			} else if(y >= 20)
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_DIRT];
			} else
			{
				map->tiles[y * MAP_WIDTH + x] = &tiles[TILE_NOTHING];
			}
		}
	}
}