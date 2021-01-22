#pragma once

/*
----- INVENTORY -----

The inventory menu and API functions.
*/

#include <stdbool.h>
#include <gint/display.h>

#include "defs.h"
#include "menu.h"
#include "chest.h"

#define NUM_PICKS 2
#define NUM_SWORDS 2
#define NUM_ARMOUR 6

enum Items {
	ITEM_NULL,
	ITEM_STONE,
	ITEM_DIRT,
	ITEM_WOOD,
	ITEM_WBENCH,
	ITEM_PLATFORM,
	ITEM_CHAIR,
	ITEM_COPPER_SWORD,
	ITEM_COPPER_PICK,
	ITEM_GEL,
	ITEM_TORCH,
	ITEM_FURNACE,
	ITEM_IRON_ORE,
	ITEM_IRON_BAR,
	ITEM_ANVIL,
	ITEM_CHEST,
	ITEM_DOOR,
	ITEM_IRON_HELMET,
	ITEM_IRON_CHAINMAIL,
	ITEM_IRON_GREAVES,
	ITEM_WOOD_HELMET,
	ITEM_WOOD_BREASTPLATE,
	ITEM_WOOD_GREAVES,
	ITEM_IRON_SWORD,
	ITEM_IRON_PICK,
	ITEM_SHACKLE,
	ITEM_CLOUD_BOTTLE,
	ITEM_MAGIC_MIRROR,
	ITEM_AGLET,
	ITEM_SAND,
	ITEM_EMPTY_BUCKET,
	ITEM_WATER_BUCKET,

	ITEMS_COUNT
};

typedef struct {
	enum Items id;
	int amount;
} Item;

struct Chest {
	int topX;
	int topY;
	Item items[INVENTORY_SIZE];
};

enum ArmourSlots {
	ARMOUR_HELMET,
	ARMOUR_TORSO,
	ARMOUR_PANTS
};

struct Inventory {
	Item items[INVENTORY_SIZE];
	Item armour[3];
	Item accessories[5];
	int hotbarSlot;
	int ticksSinceInteracted;

	int (*getFirstFreeSlot)(enum Items item);
	void (*removeItem)(Item *item);
	void (*stackItem)(Item *dest, Item *source);
	int (*tallyItem)(enum Items item);
	int (*findSlot)(enum Items item);
	Item *(*getSelected)();
};

struct PickData {
	enum Items item;
	int power;
	int speed;
	int knockback;
	int damage;
	bopti_image_t *swingSprite;
	int currFramesLeft;
	struct {
		int x;
		int y;
		int damage;
		int crackVar;
	} targeted;
};

struct SwordData {
	enum Items item;
	int knockback;
	int damage;
	bopti_image_t *swingSprite;
};

struct ArmourData {
	enum Items item;
	int defense;
};

extern const struct PickData pickData[NUM_PICKS];
extern const struct SwordData swordData[NUM_SWORDS];
extern const struct ArmourData armourData[NUM_ARMOUR];

enum ToolTypes {
	TOOL_TYPE_NONE = -1,
	TOOL_TYPE_SWORD,
	TOOL_TYPE_PICK,
	TOOL_TYPE_ACCESSORY,
	TOOL_TYPE_HELMET,
	TOOL_TYPE_TORSO,
	TOOL_TYPE_PANTS,
	TOOL_TYPE_OTHER
};

struct ItemData {
	int maxStack;
	int tile;
	char *name;
	enum ToolTypes type;
};

extern const struct ItemData items[];

struct PlayerTool {
	enum ToolTypes type;
	union {
		struct PickData pickData;
		struct SwordData swordData;
	} data;
};

/* getFirstFreeSlot
Tries to find the first free slot in the player's inventory, with either
nothing in it or a less-than-max stack of the given item type.

item: Item type to take into consideration when looking for a slot.

Returns the index of the free slot, or -1 if none exist.
*/
int getFirstFreeSlot(enum Items item);

/* findSlot
Finds the first slot containing the given item type.

item: The Item type to find.

Returns the index of the first slot containing that item, or -1 if none exist.
*/
int findSlot(enum Items item);

/* removeItem
Removes a single item from the stack at the given slot. Works even if no item
is present in the slot.

item: Pointer to the item to decrement.
*/
void removeItem(Item *item);

/* stackItem
Stacks the source item onto the destination item. Does nothing if the types
are different (unless destination type is ITEM_NULL). If there are too
many items in the source stack, it will only stack as many as possible.

dest: Destination item to be stacked onto
source: Source item to stack
*/
void stackItem(Item *dest, Item *source);

/* tallyItem
Tallies the number of items with the given type in the player's inventory.

item: The item type to tally.

Returns the number of matching items found.
*/
int tallyItem(enum Items item);

/* getSelected
Gets the selected item in the hotbar.

Returns a pointer to the selected item.
*/
Item *getSelected();

/* inventoryMenu
The inventory menu.

chest: Pointer to a chest, can be NULL.
*/
void inventoryMenu(struct Chest* chest);

/* registerEquipment
Applies all effects of armour and equipment on the player.
*/
void registerEquipment();

/* registerHeld
Updates tool data for the held tool.
*/
void registerHeld();

/* registerGhost
Applied the appropriate ghost outline for the held item.
*/
void registerGhost();