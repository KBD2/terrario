#include "inventory.h"
#include "world.h"

extern bopti_image_t
img_item_null,
img_item_dirt,
img_item_stone;

const ItemData items[] = {
//		Sprite				Max		Tile
	{	&img_item_null	,	0	,	TILE_NULL	},	// ITEM_NULL
	{	&img_item_stone	,	99	,	TILE_STONE	},	// ITEM_STONE
	{	&img_item_dirt	,	99	,	TILE_DIRT	}	// ITEM_DIRT
};