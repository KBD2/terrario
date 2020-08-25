#include <gint/keyboard.h>
#include <gint/gray.h>
#include <gint/timer.h>
#include <gint/defs/util.h>
#include <gint/gint.h>

#include "inventory.h"
#include "world.h"
#include "entity.h"

extern bopti_image_t
img_item_null,
img_item_dirt,
img_item_stone,
img_item_wood,
img_item_wbench,
img_item_platform,
img_item_chair,
img_item_sword;

const ItemData items[] = {
//		Sprite				Max		Tile
	{	&img_item_null,		0,		TILE_NULL,		"Null",			false	},	// ITEM_NULL
	{	&img_item_stone,	999,	TILE_STONE,		"Stone",		false	},	// ITEM_STONE
	{	&img_item_dirt,		999,	TILE_DIRT,		"Dirt",			false	},	// ITEM_DIRT
	{	&img_item_wood,		999,	TILE_WOOD,		"Wood",			false	},	// ITEM_WOOD
	{	&img_item_wbench,	99,		TILE_WBENCH_L,	"Workbench",	false	},	// ITEM_WORKBENCH
	{	&img_item_platform,	999,	TILE_PLATFORM,	"Platform",		false	},	// ITEM_PLATFORM
	{	&img_item_chair,	99,		TILE_CHAIR,		"Chair",		false	},	// ITEM_CHAIR
	{	&img_item_sword,	1,		TILE_NULL,		"Sword",		true	}	// ITEM_SWORD
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
	Item *item = &player.inventory.items[slot];

	item->number--;
	if(item->number == 0) *item = (Item){ITEM_NULL, 0};
}

void stackItem(Item *dest, Item *source)
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

int tallyItem(enum Items item)
{
	int count = 0;

	for(int slot = 0; slot < INVENTORY_SIZE; slot++)
	{
		if(player.inventory.items[slot].id == item) count += player.inventory.items[slot].number;
	}

	return count;
}

Item *getSelected()
{
	return &player.inventory.items[player.inventory.hotbarSlot];
}

int inventoryKeyFilter(int key, GUNUSED int duration, GUNUSED int count)
{
	if(key == KEY_F1) return -1;
	return 0;
}

void inventoryMenu()
{
	extern bopti_image_t img_inventory, img_cursor;
	Item *item;
	int hoverSlot;
	int cursorX = 64, cursorY = 32;
	Item held = {ITEM_NULL, 0};
	int freeSlot;
	key_event_t key;

	getkey_repeat_filter(&inventoryKeyFilter);

	while(true)
	{
		render();
		dimage(0, 0, &img_inventory);
		for(int slot = 0; slot < INVENTORY_SIZE; slot++)
		{
			item = &player.inventory.items[slot];
			if(item->id != ITEM_NULL)
			{
				renderItem((slot % 8) * 16 + 1, (slot / 8) * 17 + 1, item);
			}
		}
		if(held.id != ITEM_NULL)
		{
			renderItem(cursorX - 7, min(35, cursorY - 7), &held);
		}
		dimage(cursorX - 2, cursorY - 2, &img_cursor);
		dupdate();

		key = getkey_opt(GETKEY_REP_ALL | GETKEY_REP_FILTER, NULL);
		hoverSlot = (cursorY / 17) * 8 + (cursorX / 16);
		item = &player.inventory.items[hoverSlot];
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_switch(&takeVRAMCapture);
				break;

			case KEY_SHIFT: case KEY_ALPHA:
				if(key.type == KEYEV_DOWN)
				{
					while(held.id != ITEM_NULL)
					{
						freeSlot = player.inventory.getFirstFreeSlot(held.id);
						if(freeSlot > -1)
						{
							player.inventory.stackItem(&player.inventory.items[freeSlot], &held);
						}
						else break;
					}
					getkey_repeat_filter(NULL);
					return;
				}
				break;

			case KEY_LEFT:
				cursorX -= 2;
				break;
			case KEY_RIGHT:
				cursorX += 2;
				break;
			case KEY_UP:
				cursorY -= 2;
				break;
			case KEY_DOWN:
				cursorY += 2;
				break;

			case KEY_F1:
				if(item->id == held.id)
				{
					player.inventory.stackItem(item, &held);
				}
				else
				{
					swap(*item, held);
				}
				break;
			case KEY_F2:
				if((item->id == held.id && held.number < items[held.id].maxStack) || held.id == ITEM_NULL)
				{
					player.inventory.stackItem(&held, &(Item){item->id, 1});
					player.inventory.removeItem(hoverSlot);
				}
				break;

			case KEY_DEL:
				*item = (Item){ITEM_NULL, 0};
				break;

			default:
				break;
		}
		cursorX = min(max(cursorX, 2), 125);
		cursorY = min(max(cursorY, 2), 48);
	}
}