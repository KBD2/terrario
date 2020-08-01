#include "inventory.h"
#include "world.h"
#include "entity.h"

extern bopti_image_t
img_item_null,
img_item_dirt,
img_item_stone,
img_item_wood;

const ItemData items[] = {
//		Sprite				Max		Tile
	{	&img_item_null	,	0	,	TILE_NULL	,	"Null"	},	// ITEM_NULL
	{	&img_item_stone	,	999	,	TILE_STONE	,	"Stone"	},	// ITEM_STONE
	{	&img_item_dirt	,	999	,	TILE_DIRT	,	"Dirt"	},	// ITEM_DIRT
	{	&img_item_wood	,	999	,	TILE_WOOD	,	"Wood"	}	// ITEM_WOOD
};

int getFirstFreeSlot(enum Items item)
{
	Item check;
	int firstEmptySlot = -1;

	for(int slot = 0; slot < INVENTORY_SIZE; slot++) 
	{
		check = player.inventory.items[slot];
		if(check.id == ITEM_NULL && firstEmptySlot == -1)
		{
			firstEmptySlot = slot;
		} 
		if(check.id == item && check.number < items[check.id].maxStack) return slot;
	}

	if(firstEmptySlot > -1) return firstEmptySlot;

	return -1;
}

// Might be needed later on
int findSlot(enum Items item)
{
	for(int slot = 0; slot < INVENTORY_SIZE; slot++)
	{
		if(player.inventory.items[slot].id == item) return slot;
	}

	return -1;
}

void removeItem(int slot)
{
	Item* item = &player.inventory.items[slot];

	item->number--;
	if(item->number == 0) *item = (Item){ITEM_NULL, 0};
}

void stackItem(Item* dest, Item* source)
{
	if(dest->id == ITEM_NULL)
	{
		*dest = *source;
		*source = (Item){ITEM_NULL, 0};
	}

	if(dest->id != source->id) return;

	if(dest->number + source->number <= items[dest->id].maxStack)
	{
		dest->number += source->number;
		*source = (Item){ITEM_NULL, 0};
	}
	else
	{
		source->number -= (items[dest->id].maxStack - dest->number);
		dest->number = items[dest->id].maxStack;
	}
}