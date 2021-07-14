#include <gint/keyboard.h>
#include <gint/gray.h>
#include <gint/timer.h>
#include <gint/defs/util.h>
#include <gint/gint.h>

#include "inventory.h"
#include "world.h"
#include "entity.h"

const struct ItemData items[] = {
//		Max		Tier 	Tile				Name				Tool type
	{	0,		0,		TILE_NULL,			"Null",					TOOL_TYPE_NONE		},	// ITEM_NULL
	{	999,	0,		TILE_STONE,			"Stone",				TOOL_TYPE_NONE		},	// ITEM_STONE
	{	999,	0,		TILE_DIRT,			"Dirt",					TOOL_TYPE_NONE		},	// ITEM_DIRT
	{	999,	0,		TILE_WOOD,			"Wood",					TOOL_TYPE_NONE		},	// ITEM_WOOD
	{	99,		0,		TILE_WBENCH_L,		"Workbench",			TOOL_TYPE_NONE		},	// ITEM_WORKBENCH
	{	999,	0,		TILE_PLATFORM,		"Platform",				TOOL_TYPE_NONE		},	// ITEM_PLATFORM
	{	99,		0,		TILE_CHAIR_L,		"Chair",				TOOL_TYPE_NONE		},	// ITEM_CHAIR
	{	1,		0,		TILE_NULL,			"Copper Sword",			TOOL_TYPE_SWORD		},	// ITEM_COPPER_SWORD
	{	1,		0,		TILE_NULL,			"Copper Pick",			TOOL_TYPE_PICK		},	// ITEM_COPPER_PICK
	{	999,	0,		TILE_NULL,			"Gel",					TOOL_TYPE_NONE		},	// ITEM_GEL
	{	99,		0,		TILE_TORCH,			"Torch",				TOOL_TYPE_NONE		},	// ITEM_TORCH
	{	99,		0,		TILE_FURNACE_EDGE,	"Furnace",				TOOL_TYPE_NONE		},	// ITEM_FURNACE
	{	999,	0,		TILE_IRON_ORE,		"Iron Ore",				TOOL_TYPE_NONE		},	// ITEM_IRON_ORE
	{	999,	0,		TILE_NULL,			"Iron Bar",				TOOL_TYPE_NONE		},	// ITEM_IRON_BAR
	{	99,		0,		TILE_ANVIL_L,		"Anvil",				TOOL_TYPE_NONE		},	// ITEM_ANVIL
	{	99,		0,		TILE_CHEST_L,		"Chest",				TOOL_TYPE_NONE		},	// ITEM_CHEST
	{	99,		0,		TILE_DOOR_C,		"Door",					TOOL_TYPE_NONE		},	// ITEM_DOOR
	{	1,		0,		TILE_NULL,			"Iron Helmet",			TOOL_TYPE_HELMET	},	// ITEM_IRON_HELMET
	{	1,		0,		TILE_NULL,			"Iron Chainmail",		TOOL_TYPE_TORSO		},	// ITEM_IRON_CHAINMAIL
	{	1,		0,		TILE_NULL,			"Iron Greaves",			TOOL_TYPE_PANTS		},	// ITEM_IRON_GREAVES
	{	1,		0,		TILE_NULL,			"Wood Helmet",			TOOL_TYPE_HELMET	},	// ITEM_WOOD_HELMET
	{	1,		0,		TILE_NULL,			"Wood Breastplate",		TOOL_TYPE_TORSO		},	// ITEM_WOOD_BREASTPLATE
	{	1,		0,		TILE_NULL,			"Wood Greaves",			TOOL_TYPE_PANTS		},	// ITEM_WOOD_GREAVES
	{	1,		0,		TILE_NULL,			"Iron Sword",			TOOL_TYPE_SWORD		},	// ITEM_IRON_SWORD
	{	1,		0,		TILE_NULL,			"Iron Pick",			TOOL_TYPE_PICK		},	// ITEM_IRON_PICK
	{	1,		0,		TILE_NULL,			"Shackle",				TOOL_TYPE_ACCESSORY	},	// ITEM_SHACKLE
	{	1, 		0,		TILE_NULL,			"Cloud in a Bottle",	TOOL_TYPE_ACCESSORY	},	// ITEM_CLOUD_BOTTLE
	{	1,		0,		TILE_NULL,			"Magic Mirror",			TOOL_TYPE_OTHER		},	// ITEM_MAGIC_MIRROR
	{	1,		0,		TILE_NULL,			"Aglet",				TOOL_TYPE_ACCESSORY	},	// ITEM_AGLET
	{	999,	0,		TILE_SAND,			"Sand",					TOOL_TYPE_NONE		},	// ITEM_SAND
	{	99,		0,		TILE_NULL,			"Empty Bucket",			TOOL_TYPE_NONE		},	// ITEM_EMPTY_BUCKET
	{	1,		0,		TILE_WATER,			"Water Bucket",			TOOL_TYPE_NONE		},	// ITEM_WATER_BUCKET
	{	99,		0,		TILE_CRYST_L,		"Life Crystal",			TOOL_TYPE_OTHER		},	// ITEM_CRYST

	{	1,		0,		TILE_NULL,			"Tin Helmet",			TOOL_TYPE_HELMET	},	// ITEM_TIN_HELMET
	{	1,		0,		TILE_NULL,			"Tin Chainmail",		TOOL_TYPE_TORSO		},	// ITEM_TIN_CHAINMAIL
	{	1,		0,		TILE_NULL,			"Tin Greaves",			TOOL_TYPE_PANTS		},	// ITEM_TIN_GREAVES
	{	1,		0,		TILE_NULL,			"Tin Sword",			TOOL_TYPE_SWORD		},	// ITEM_TIN_SWORD
	{	1,		0,		TILE_NULL,			"Tin Pick",				TOOL_TYPE_PICK		},	// ITEM_TIN_PICK
	{	999,	0,		TILE_MUD,			"Mud",					TOOL_TYPE_NONE		},	// ITEM_MUD
	{	999,	0,		TILE_CLAY,			"Clay",					TOOL_TYPE_NONE		},	// ITEM_CLAY
	{	999,	0,		TILE_COPPER_ORE,	"Copper Ore",			TOOL_TYPE_NONE		},	// ITEM_COPPER_ORE
	{	999,	0,		TILE_NULL,			"Copper Bar",			TOOL_TYPE_NONE		},	// ITEM_COPPER_BAR
	{	999,	0,		TILE_TIN_ORE,		"Tin Ore",				TOOL_TYPE_NONE		},	// ITEM_TIN_ORE
	{	999,	0,		TILE_NULL,			"Tin Bar",				TOOL_TYPE_NONE		},	// ITEM_TIN_BAR
	{	100,	0,		TILE_NULL,			"Copper Coin",			TOOL_TYPE_NONE		},	// ITEM_COIN_COPPER
	{	100,	0,		TILE_NULL,			"Silver Coin",			TOOL_TYPE_NONE		},	// ITEM_COIN_SILVER
	{	100,	0,		TILE_NULL,			"Gold Coin",			TOOL_TYPE_NONE		},	// ITEM_COIN_GOLD
	{	100,	0,		TILE_NULL,			"Platinum Coin",		TOOL_TYPE_NONE		},	// ITEM_COIN_PLATINUM
	{	99,		0,		TILE_NULL,			"Lens",					TOOL_TYPE_NONE		},	// ITEM_LENS
	{	999,	0,		TILE_CACTUS,		"Cactus",				TOOL_TYPE_NONE		},	// ITEM_CACTUS
	{	1,		0,		TILE_NULL,			"Cactus Helmet",		TOOL_TYPE_HELMET	},	// ITEM_CACTUS_HELMET
	{	1,		0,		TILE_NULL,			"Cactus Breastplate",	TOOL_TYPE_TORSO		},	// ITEM_CACTUS_BREASTPLATE
	{	1,		0,		TILE_NULL,			"Cactus Greaves",		TOOL_TYPE_PANTS		},	// ITEM_CACTUS_GREAVES
	{	1,		0,		TILE_NULL,			"Cactus Sword",			TOOL_TYPE_SWORD		},	// ITEM_CACTUS_SWORD
	{	1,		0,		TILE_NULL,			"Cactus Pick",			TOOL_TYPE_PICK		},	// ITEM_CACTUS_PICK
	{	1,		0,		TILE_NULL,			"Zombie Arm",			TOOL_TYPE_SWORD		},	// ITEM_ZOMBIE_ARM
	{	999,	0,		TILE_MUSHROOM,		"Mushroom",				TOOL_TYPE_NONE		},	// ITEM_MUSHROOM
	{	999,	0,		TILE_NULL,			"Glass",				TOOL_TYPE_NONE		},	// ITEM_GLASS
	{	999,	0,		TILE_NULL,			"Bottle",				TOOL_TYPE_NONE		},	// ITEM_BOTTLE
	{	30,		0,		TILE_NULL,			"Lesser Healing Potion",TOOL_TYPE_NONE		},	// ITEM_LESSER_HEALING_POTION
	{	30,		0,		TILE_NULL,			"Lesser Mana Potion",	TOOL_TYPE_NONE		},	// ITEM_LESSER_MANA_POTION
};

extern bopti_image_t img_swing_copper_pick, img_swing_iron_pick, img_swing_tin_pick, img_swing_cactus_pick;

const struct PickData pickData[NUM_PICKS] = {
	{.item = ITEM_COPPER_PICK,	.power = 35,	.speed = 15,	.knockback = 2,	.damage = 4,	.swingSprite = &img_swing_copper_pick},
	{.item = ITEM_IRON_PICK,	.power = 40,	.speed = 13,	.knockback = 2,	.damage = 5,	.swingSprite = &img_swing_iron_pick},
	{.item = ITEM_TIN_PICK,		.power = 35,	.speed = 15,	.knockback = 2,	.damage = 5,	.swingSprite = &img_swing_tin_pick},
	{.item = ITEM_CACTUS_PICK,	.power = 35,	.speed = 16,	.knockback = 2,	.damage = 4,	.swingSprite = &img_swing_cactus_pick},
};

extern bopti_image_t img_swing_zombie_arm, img_swing_copper_sword, img_swing_iron_sword, img_swing_tin_sword, img_swing_cactus_sword;

const struct SwordData swordData[NUM_SWORDS] = {
	{.item = ITEM_ZOMBIE_ARM,	.knockback = 4,	.damage = 12,	.swingSprite = &img_swing_zombie_arm},
	{.item = ITEM_COPPER_SWORD,	.knockback = 5,	.damage = 8,	.swingSprite = &img_swing_copper_sword},
	{.item = ITEM_IRON_SWORD,	.knockback = 5,	.damage = 10,	.swingSprite = &img_swing_iron_sword},
	{.item = ITEM_TIN_SWORD,	.knockback = 5,	.damage = 9,	.swingSprite = &img_swing_tin_sword},
	{.item = ITEM_CACTUS_SWORD,	.knockback = 5,	.damage = 8,	.swingSprite = &img_swing_cactus_sword},
};

const struct ArmourData armourData[NUM_ARMOUR] = {
	{.item = ITEM_IRON_HELMET,			.defense = 2},
	{.item = ITEM_IRON_CHAINMAIL,		.defense = 2},
	{.item = ITEM_IRON_GREAVES,			.defense = 2},
	{.item = ITEM_WOOD_HELMET,			.defense = 1},
	{.item = ITEM_WOOD_BREASTPLATE,		.defense = 1},
	{.item = ITEM_WOOD_GREAVES,			.defense = 0},
	{.item = ITEM_TIN_HELMET,			.defense = 2},
	{.item = ITEM_TIN_CHAINMAIL,		.defense = 2},
	{.item = ITEM_TIN_GREAVES,			.defense = 1},
	{.item = ITEM_CACTUS_HELMET,		.defense = 1},
	{.item = ITEM_CACTUS_BREASTPLATE,	.defense = 2},
	{.item = ITEM_CACTUS_GREAVES,		.defense = 1},
};

const struct ItemPrefixData prefixes[PREFIX_COUNT] = {
//		Name			Tier 	Dmg/Def 	Knockback	Speed 	Power
	{	"",				0,		1.00,		1.00,		1.00,	1.00 	}, // PREFIX_NONE
	{	"Keen ",		1,		1.05,		1.00,		1.05,	1.05 	}, // PREFIX_KEEN
	{	"Superior ",	2,		1.10,		1.10,		1.00,	1.10 	}, // PREFIX_SUPERIOR
	{	"Forceful ",	1,		1.00,		1.15,		1.00,	1.05 	}, // PREFIX_FORCEFUL
	{	"Broken ",		-2,		0.70,		0.80,		1.00,	0.95 	}, // PREFIX_BROKEN
	{	"Damaged ",		-1,		0.85,		1.00,		1.00,	1.00 	}, // PREFIX_DAMAGED
	{	"Shoddy ",		-2,		0.90,		0.85,		1.00,	0.95 	}, // PREFIX_SHODDY
	{	"Hurtful ",		1,		1.10,		1.00,		1.00,	1.05 	}, // PREFIX_HURTFUL
	{	"Strong ",		1,		1.00,		1.15,		1.00,	1.05 	}, // PREFIX_STRONG
	{	"Unpleasant ",	2,		1.05,		1.15,		1.00,	1.10 	}, // PREFIX_UNPLEASANT
	{	"Weak ",		-1,		1.00,		0.80,		1.00,	1.00 	}, // PREFIX_WEAK
	{	"Ruthless ",	1,		1.20,		0.90,		1.00,	1.05 	}, // PREFIX_RUTHLESS
	{	"Godly ",		2,		1.20,		1.15,		1.00,	1.10 	}, // PREFIX_GODLY
	{	"Demonic ",		2,		1.20,		1.00,		1.00,	1.10 	}, // PREFIX_DEMONIC
	{	"Zealous ",		1,		1.10,		1.00,		1.00,	1.05 	}, // PREFIX_ZEALOUS
};

enum InventoryTabs {
	TAB_MAIN,
	TAB_EQUIP,
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
	if(item->amount <= 0) *item = (Item){ITEM_NULL, PREFIX_NONE, 0};
}

void stackItem(Item *dest, Item *source)
{
	if(dest->id == ITEM_NULL)
	{
		*dest = *source;
		*source = (Item){ITEM_NULL, PREFIX_NONE, 0};
		return;
	}

	if(dest->id != source->id) return;

	if(dest->amount + source->amount <= items[dest->id].maxStack)
	{
		dest->amount += source->amount;
		*source = (Item){ITEM_NULL, PREFIX_NONE, 0};
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

int inventoryKeyProfile(int key, GUNUSED int duration, GUNUSED int count)
{
        if(key == KEY_F1) return -1;
		if(key == KEY_F2) return 300000;
        return 16000;
}

/*
Equipment slots:
0-4: Accessories, 7: Helmet, 15: Torso, 23: Pants
It's this way because it's easier to treat the equipment inventory like the others
and just disable interaction with all other 'slots'.
*/
void inventoryMenu(struct Chest* chest)
{
	extern bopti_image_t img_ui_slots, img_ui_slot_highlight, img_ui_cursor, img_ui_inventory_tabs, img_ui_placeholders;
	Item *item;
	int hoverSlot;
	int cursorX = 64, cursorY = 32;
	Item held = {ITEM_NULL, 0};
	int freeSlot;
	key_event_t key;
	int width, height;
	int tab = (chest == NULL) ? TAB_MAIN : TAB_CHEST;

	getkey_repeat(16, 16);
	getkey_set_repeat_profile(&inventoryKeyProfile);	

	while(true)
	{
		render(false);
		if(chest != NULL)
		{
			if(tab == TAB_CHEST) dsubimage(67, 55, &img_ui_inventory_tabs, 20, 18, 19, 9, DIMAGE_NONE);
			else dsubimage(67, 55, &img_ui_inventory_tabs, 0, 18, 19, 9, DIMAGE_NONE);
		}

		if(tab == TAB_MAIN) dsubimage(86, 55, &img_ui_inventory_tabs, 20, 9, 21, 9, DIMAGE_NONE);
		else dsubimage(86, 55, &img_ui_inventory_tabs, 0, 9, 21, 9, DIMAGE_NONE);

		if(tab == TAB_EQUIP) dsubimage(107, 55, &img_ui_inventory_tabs, 20, 0, 21, 9, DIMAGE_NONE);
		else dsubimage(107, 55, &img_ui_inventory_tabs, 0, 0, 21, 9, DIMAGE_NONE);

//		Render all items in the tabbed storage
		if(tab == TAB_EQUIP)
		{
			dsubimage(0, 0, &img_ui_slots, 0, 0, 80, 17, DIMAGE_NONE);
			dsubimage(112, 0, &img_ui_slots, 0, 0, 16, 51, DIMAGE_NONE);
			for(int slot = 0; slot < 5; slot++)
			{
				item = &player.inventory.accessories[slot];
				if(item->id == ITEM_NULL) dsubimage(slot * 16 + 1, 2, &img_ui_placeholders, 46, 1, 14, 13, DIMAGE_NOCLIP);
				else renderItem(slot * 16 + 1, 1, item);
			}
			for(int slot = 0; slot < 3; slot++)
			{
				item = &player.inventory.armour[slot];
				if(item->id == ITEM_NULL) dsubimage(113, slot * 17 + 2, &img_ui_placeholders, slot * 15 + 1, 1, 14, 13, DIMAGE_NOCLIP);
				else renderItem(113, slot * 17 + 1, item);
			}
			dprint_opt(112, 0, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "%i DEF", player.combat.defense + player.bonuses.defense);
		}
		else
		{
			dimage(0, 0, &img_ui_slots);
			if(tab == TAB_MAIN)
			{
				for(int slot = 0; slot < 5; slot++) dimage(16 * slot, 0, &img_ui_slot_highlight);
			}
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

		dimage(cursorX - 2, cursorY - 2, &img_ui_cursor);
		hoverSlot = (cursorY / 17) * 8 + (cursorX / 16);
		switch(tab)
		{
			case TAB_MAIN:
				item = &player.inventory.items[hoverSlot];
				break;
			case TAB_CHEST:
				item = &chest->items[hoverSlot];
				break;
			case TAB_EQUIP:
				if(hoverSlot < 5) item = &player.inventory.accessories[hoverSlot];
				else if(hoverSlot % 8 == 7) item = &player.inventory.armour[hoverSlot / 8];
				else item = NULL;
				break;
		}
		if(item != NULL && item->id != ITEM_NULL)
		{
			int prefix = PREFIX_NONE;

			if (items[item->id].type != TOOL_TYPE_NONE)
			{
				prefix = item->prefix;
			}

			int prefixWidth;

			dsize(items[item->id].name, NULL, &width, &height);
			dsize(prefixes[prefix].name, NULL, &prefixWidth, NULL);

			drect(0, 51, width + prefixWidth, 52 + height, C_WHITE);
			dprint(0, 52, C_BLACK, "%s%s", prefixes[prefix].name, items[item->id].name);
		}
		dupdate();

		key = getkey_opt(GETKEY_REP_PROFILE, NULL);
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_world_switch(GINT_CALL(&takeVRAMCapture));
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
					getkey_repeat(400, 40);
					getkey_set_repeat_profile(NULL);
					return;
				}
				break;

			case KEY_LEFT:
				cursorX -= 1;
				break;
			case KEY_RIGHT:
				cursorX += 1;
				break;
			case KEY_UP:
				cursorY -= 1;
				break;
			case KEY_DOWN:
				cursorY += 1;
				break;

			case KEY_F1:
				if(item == NULL) break;
				if(tab == TAB_EQUIP && held.id != ITEM_NULL)
				{
					if(hoverSlot < 5 && items[held.id].type != TOOL_TYPE_ACCESSORY) break;
					if(hoverSlot == 7 && items[held.id].type != TOOL_TYPE_HELMET) break;
					if(hoverSlot == 15 && items[held.id].type != TOOL_TYPE_TORSO) break;
					if(hoverSlot == 23 && items[held.id].type != TOOL_TYPE_PANTS) break;
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
//				Recalculate defense and stuff
				registerEquipment();
				break;
			case KEY_F2:
				if(tab == TAB_EQUIP) break;
				if(item != NULL && (item->id == held.id || held.id == ITEM_NULL))
				{
					player.inventory.stackItem(&held, &(Item){item->id, item->prefix, 1});
//					Also storage-agnostic
					player.inventory.removeItem(item);
				}
				break;
			
			case KEY_F6:
				tab = TAB_EQUIP;
				break;
			case KEY_F5:
				tab = TAB_MAIN;
				break;
			case KEY_F4:
				if(chest != NULL) tab = TAB_CHEST;
				break;

			case KEY_DEL:
				*item = (Item){ITEM_NULL, PREFIX_NONE, 0};
				break;

			default:
				break;
		}
		cursorX = min(max(cursorX, 2), 125);
		cursorY = min(max(cursorY, 2), 48);
	}
}

void registerEquipment()
{
	enum Items armour[3];
	enum Items part;

	player.combat.defense = 0;
	for(int slot = 0; slot < 3; slot++)
	{
		part = player.inventory.armour[slot].id;
		armour[slot] = part;
		if(part == ITEM_NULL || part < 0 || part >= ITEMS_COUNT) continue;
		for(int idx = 0; idx < NUM_ARMOUR; idx++)
		{
			if(armourData[idx].item == part)
			{
				player.combat.defense += (float)(armourData[idx].defense) * prefixes[player.inventory.armour[slot].prefix].dmgdef;
				break;
			}
		}
	}

	player.bonuses = (struct AccessoryBonuses){ 0 };
	player.bonuses.speedBonus = 1;

	for(int slot = 0; slot < 5; slot++)
	{
		if(player.inventory.accessories[slot].id == ITEM_NULL) continue;
		switch(player.inventory.accessories[slot].id)
		{

//			----- ACCESSORY BONUSES -----

			case ITEM_SHACKLE:
				player.bonuses.defense++;
				break;
			
			case ITEM_CLOUD_BOTTLE:
				player.bonuses.doubleJump = true;
				break;
			
			case ITEM_AGLET:
				player.bonuses.speedBonus += 0.05;

			default:
				break;
		}
	}

//	----- ARMOUR SET BONUSES -----

//	Iron Armour
	if(armour[0] == ITEM_IRON_HELMET && armour[1] == ITEM_IRON_CHAINMAIL && armour[2] == ITEM_IRON_GREAVES)
	{
		player.combat.defense += 2;
	}
//	Tin Armour
	if(armour[0] == ITEM_TIN_HELMET && armour[1] == ITEM_TIN_CHAINMAIL && armour[2] == ITEM_TIN_GREAVES)
	{
		player.combat.defense += 2;
	}
//	Wood Armour
	if(armour[0] == ITEM_WOOD_HELMET && armour[1] == ITEM_WOOD_BREASTPLATE && armour[2] == ITEM_WOOD_GREAVES)
	{
		player.combat.defense++;
	}
}

void registerHeld()
{
	unsigned int currID = player.inventory.getSelected()->id;

	switch(items[currID].type)
	{
		case TOOL_TYPE_PICK:
			player.tool.type = TOOL_TYPE_PICK;
			for(int i = 0; i < NUM_PICKS; i++)
			{
				if(pickData[i].item == currID) player.tool.data.pickData = pickData[i];
				player.combat.attack = player.tool.data.pickData.damage * prefixes[player.inventory.getSelected()->prefix].dmgdef;
			}
			break;
					
		case TOOL_TYPE_SWORD:
			player.tool.type = TOOL_TYPE_SWORD;
			for(int i = 0; i < NUM_SWORDS; i++)
			{
				if(swordData[i].item == currID) player.tool.data.swordData = swordData[i];
				player.combat.attack = player.tool.data.swordData.damage * prefixes[player.inventory.getSelected()->prefix].dmgdef;
			}
			break;

		default:
			player.tool.type = TOOL_TYPE_NONE;
			break;
	}
	registerGhost();
}

void registerGhost()
{
	unsigned int currID = player.inventory.getSelected()->id;

	switch(currID)
	{
		case ITEM_WBENCH:
		case ITEM_ANVIL:
			player.ghost = (struct GhostObject){2, 1};
			break;

		case ITEM_CHAIR:
			player.ghost = (struct GhostObject){1, 2};
			break;
		
		case ITEM_FURNACE:
			player.ghost = (struct GhostObject){3, 2};
			break;
		
		case ITEM_CHEST:
			player.ghost = (struct GhostObject){2, 2};
			break;
		
		case ITEM_DOOR:
			player.ghost = (struct GhostObject){1, 3};
			break;
		
		default:
			player.ghost = (struct GhostObject){0, 0};
			break;
	}
}
