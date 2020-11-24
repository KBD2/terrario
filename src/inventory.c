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
img_item_copper_sword,
img_item_copper_pick,
img_item_gel,
img_item_torch,
img_item_furnace,
img_item_iron_ore,
img_item_iron_bar,
img_item_anvil,
img_item_chest,
img_item_door;

const ItemData items[] = {
//		Sprite				Max			Tile								Tool type?
	{	&img_item_null,			0,		TILE_NULL,			"Null",			TOOL_TYPE_NONE	},	// ITEM_NULL
	{	&img_item_stone,		999,	TILE_STONE,			"Stone",		TOOL_TYPE_NONE	},	// ITEM_STONE
	{	&img_item_dirt,			999,	TILE_DIRT,			"Dirt",			TOOL_TYPE_NONE	},	// ITEM_DIRT
	{	&img_item_wood,			999,	TILE_WOOD,			"Wood",			TOOL_TYPE_NONE	},	// ITEM_WOOD
	{	&img_item_wbench,		99,		TILE_WBENCH_L,		"Workbench",	TOOL_TYPE_NONE	},	// ITEM_WORKBENCH
	{	&img_item_platform,		999,	TILE_PLATFORM,		"Platform",		TOOL_TYPE_NONE	},	// ITEM_PLATFORM
	{	&img_item_chair,		99,		TILE_CHAIR_L,		"Chair",		TOOL_TYPE_NONE	},	// ITEM_CHAIR
	{	&img_item_copper_sword,	1,		TILE_NULL,			"Copper Sword",	TOOL_TYPE_SWORD	},	// ITEM_COPPER_SWORD
	{	&img_item_copper_pick,	1,		TILE_NULL,			"Copper Pick",	TOOL_TYPE_PICK	},	// ITEM_COPPER_PICK
	{	&img_item_gel,			999,	TILE_NULL,			"Gel",			TOOL_TYPE_NONE	},	// ITEM_GEL
	{	&img_item_torch,		99,		TILE_TORCH,			"Torch",		TOOL_TYPE_NONE	},	// ITEM_TORCH
	{	&img_item_furnace,		99,		TILE_FURNACE_EDGE,	"Furnace",		TOOL_TYPE_NONE	},	// ITEM_FURNACE
	{	&img_item_iron_ore,		999,	TILE_IRON_ORE,		"Iron Ore",		TOOL_TYPE_NONE	},	// ITEM_IRON_ORE
	{	&img_item_iron_bar,		999,	TILE_NULL,			"Iron Bar",		TOOL_TYPE_NONE	},	// ITEM_IRON_BAR
	{	&img_item_anvil,		99,		TILE_ANVIL_L,		"Anvil",		TOOL_TYPE_NONE	},	// ITEM_ANVIL
	{	&img_item_chest,		99,		TILE_CHEST_L,		"Chest",		TOOL_TYPE_NONE	},	// ITEM_CHEST
	{	&img_item_door,			99,		TILE_DOOR_C,		"Door",			TOOL_TYPE_NONE	},	// ITEM_DOOR
};

const struct PickData pickData[NUM_PICKS] = {
	{.power = 35, .speed = 15, .knockback = 2, .damage = 4}
};

const struct SwordData swordData[NUM_SWORDS] = {
	{.knockback = 5, .damage = 8}
};

const int toolMap[NUM_PICKS + NUM_SWORDS][2] = {
	{ITEM_COPPER_PICK, 0},
	{ITEM_COPPER_SWORD, 0}
};

enum InventoryTabs {
	TAB_MAIN,
	TAB_ARMOUR,
	TAB_CHEST
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
		if(check.id == item && check.amount < items[check.id].maxStack) return slot;
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

void removeItem(Item *item)
{
	if(item->id == ITEM_NULL || item->amount == 0) return;

	item->amount--;
	if(item->amount <= 0) *item = (Item){ITEM_NULL, 0};
}

void stackItem(Item *dest, Item *source)
{
	if(dest->id == ITEM_NULL)
	{
		*dest = *source;
		*source = (Item){ITEM_NULL, 0};
		return;
	}

	if(dest->id != source->id) return;

	if(dest->amount + source->amount <= items[dest->id].maxStack)
	{
		dest->amount += source->amount;
		*source = (Item){ITEM_NULL, 0};
	}
	else
	{
		source->amount -= (items[dest->id].maxStack - dest->amount);
		dest->amount = items[dest->id].maxStack;
	}
}

int tallyItem(enum Items item)
{
	int count = 0;

	for(int slot = 0; slot < INVENTORY_SIZE; slot++)
	{
		if(player.inventory.items[slot].id == item) count += player.inventory.items[slot].amount;
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

void inventoryMenu(struct Chest* chest)
{
	extern bopti_image_t img_slots, img_slot_highlight, img_cursor, img_inventory_tabs;
	Item *item;
	int hoverSlot;
	int cursorX = 64, cursorY = 32;
	Item held = {ITEM_NULL, 0};
	int freeSlot;
	key_event_t key;
	int width, height;
	int tab = (chest == NULL) ? TAB_MAIN : TAB_CHEST;

	getkey_repeat_filter(&inventoryKeyFilter);

	while(true)
	{
		render(false);
		if(chest != NULL)
		{
			if(tab == TAB_CHEST) dsubimage(67, 55, &img_inventory_tabs, 20, 18, 19, 9, DIMAGE_NONE);
			else dsubimage(67, 55, &img_inventory_tabs, 0, 18, 19, 9, DIMAGE_NONE);
		}

		if(tab == TAB_MAIN) dsubimage(86, 55, &img_inventory_tabs, 20, 9, 21, 9, DIMAGE_NONE);
		else dsubimage(86, 55, &img_inventory_tabs, 0, 9, 21, 9, DIMAGE_NONE);

		if(tab == TAB_ARMOUR) dsubimage(107, 55, &img_inventory_tabs, 20, 0, 21, 9, DIMAGE_NONE);
		else dsubimage(107, 55, &img_inventory_tabs, 0, 0, 21, 9, DIMAGE_NONE);

//		Render all items in the tabbed storage
		if(tab == TAB_MAIN)
		{
			for(int slot = 0; slot < 5; slot++) dimage(16 * slot, 0, &img_slot_highlight);
		}
		if(tab == TAB_ARMOUR)
		{
			dsubimage(0, 0, &img_slots, 0, 0, 80, 17, DIMAGE_NONE);
			dsubimage(112, 0, &img_slots, 0, 0, 16, 51, DIMAGE_NONE);
			for(int slot = 0; slot < 5; slot++)
			{
				item = &player.inventory.accessories[slot];
				renderItem(slot * 16 + 1, 1, item);
			}
			for(int slot = 0; slot < 3; slot++)
			{
				item = &player.inventory.armour[slot];
				renderItem(113, slot * 17 + 1, item);
			}
		}
		else
		{
			dimage(0, 0, &img_slots);
			for(int slot = 0; slot < INVENTORY_SIZE; slot++)
			{
				if(tab == TAB_CHEST) item = &chest->items[slot];
				else item = &player.inventory.items[slot];
				if(item->id != ITEM_NULL)
				{
					renderItem((slot % 8) * 16 + 1, (slot / 8) * 17 + 1, item);
				}
			}
		}
		
		if(held.id != ITEM_NULL)
		{
			renderItem(cursorX - 7, min(35, cursorY - 7), &held);
		}

		dimage(cursorX - 2, cursorY - 2, &img_cursor);
		hoverSlot = (cursorY / 17) * 8 + (cursorX / 16);
		switch(tab)
		{
			case TAB_MAIN:
				item = &player.inventory.items[hoverSlot];
				break;
			case TAB_CHEST:
				item = &chest->items[hoverSlot];
				break;
			case TAB_ARMOUR:
				if(hoverSlot < 5) item = &player.inventory.accessories[hoverSlot];
				else if(hoverSlot % 8 == 7) item = &player.inventory.armour[hoverSlot / 8];
				else item = NULL;
				break;
		}
		if(item != NULL && item->id != ITEM_NULL)
		{
			dsize(items[item->id].name, NULL, &width, &height);
			drect(0, 51, width, 52 + height, C_WHITE);
			dtext(0, 52, C_BLACK, items[item->id].name);
		}
		dupdate();

		key = getkey_opt(GETKEY_REP_ALL | GETKEY_REP_FILTER, NULL);
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
				if(item == NULL) break;
				if(tab == TAB_ARMOUR && held.id != ITEM_NULL)
				{
					if(hoverSlot < 5 && items[held.id].type != TOOL_TYPE_ACCESSORY) break;
					if(hoverSlot > 5 && items[held.id].type != TOOL_TYPE_ARMOUR) break;
				}
				if(item->id == held.id)
				{
//					Storage-agnostic, works with the chest inventory
					player.inventory.stackItem(item, &held);
				}
				else
				{
					swap(*item, held);
				}
				break;
			case KEY_F2:
				if(tab == TAB_ARMOUR) break;
				if(item != NULL && (item->id == held.id || held.id == ITEM_NULL))
				{
					player.inventory.stackItem(&held, &(Item){item->id, 1});
//					Also storage-agnostic
					player.inventory.removeItem(item);
				}
				break;
			
			case KEY_F6:
				tab = TAB_ARMOUR;
				break;
			case KEY_F5:
				tab = TAB_MAIN;
				break;
			case KEY_F4:
				if(chest != NULL) tab = TAB_CHEST;
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