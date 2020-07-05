#include "map.h"

extern image_t img_tile_stone, img_tile_dirt;

const Tile tiles[] = {
//      Ptr to sprite       Solid?
	{   &img_tile_stone ,	true	},
	{   &img_tile_dirt  ,	true	}
};

void generateMap(struct Map* map)
{
	map->tiles = malloc(MAP_WIDTH * MAP_HEIGHT);
	for(unsigned int y = 0; y < MAP_HEIGHT; y++)
	{
		for(unsigned int x = 0; x < MAP_WIDTH; x++)
		{
			map->tiles[y * MAP_HEIGHT + x] = &(tiles[TILE_STONE]);
		}
	}
}