#include <gint/std/stdlib.h>
#include <gint/defs/util.h>
#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <stdbool.h>
#include <gint/std/string.h>

#include "defs.h"
#include "syscalls.h"
#include "world.h"
#include "entity.h"
#include "render.h"
#include "menu.h"
#include "save.h"

// Syscalls
const unsigned int sc003B[] = { SCA, SCB, SCE, 0x003B };
const unsigned int sc003C[] = { SCA, SCB, SCE, 0x003C };
const unsigned int sc0236[] = { SCA, SCB, SCE, 0x0236 };
const unsigned int sc019F[] = { SCA, SCB, SCE, 0x019F };

// Global variables
struct SaveData save;
struct World world;
struct Player player;

// Checks if the RAM we'll be using to store world tiles works
bool testRAM()
{
	unsigned int* RAMAddress = (void*)RAM_START;
	unsigned int* RAMTestAddress = (void*)0x88000000;
	*RAMAddress = 0xC0FFEE;
	if(*RAMAddress == 0xC0FFEE && *RAMAddress != *RAMTestAddress) return true;
	return false;
}

bool update()
{
	Tile* tile;
	Item* item;
	int freeSlot;
	bool validLeft, validRight, validTop, validBottom;

	clearevents();

	if(keydown(KEY_ALPHA)) gint_switch(&takeVRAMCapture);

	if(keydown(KEY_SHIFT)) inventoryMenuUpdate();

	if(keydown(KEY_F1)) player.inventory.hotbarSlot = 0;
	if(keydown(KEY_F2)) player.inventory.hotbarSlot = 1;
	if(keydown(KEY_F3)) player.inventory.hotbarSlot = 2;

	if(keydown(KEY_4)) player.props.xVel = -1;
	if(keydown(KEY_6)) player.props.xVel = 1;
	if(keydown(KEY_8) && player.props.touchingTileTop) player.props.yVel = -4.5;

	if(keydown(KEY_7))
	{
		tile = &getTile(player.cursorTile.x, player.cursorTile.y);
		if(tiles[tile->idx].item != ITEM_NULL)
		{
			freeSlot = player.inventory.getFirstFreeSlot(tiles[tile->idx].item);
			if(freeSlot > -1) player.inventory.stackItem(&player.inventory.items[freeSlot], &((Item){tiles[tile->idx].item, 1}));
		}
		*tile = (Tile){TILE_NOTHING};
		regionChange(player.cursorTile.x, player.cursorTile.y);
	}
	if(keydown(KEY_9))
	{
		validLeft = player.cursorTile.x < player.props.x >> 3;
		validRight = player.cursorTile.x > (player.props.x + player.props.width) >> 3;
		validTop = player.cursorTile.y < player.props.y >> 3;
		validBottom = player.cursorTile.y > (player.props.y + player.props.height) >> 3;
		if(validLeft || validRight || validTop || validBottom)
		{
			tile = &getTile(player.cursorTile.x, player.cursorTile.y);
			if(tile->idx == TILE_NOTHING)
			{
				item = &player.inventory.items[player.inventory.hotbarSlot];
				if(item->id != ITEM_NULL && items[item->id].tile > -1)
				{
					*tile = (Tile){items[item->id].tile};
					regionChange(player.cursorTile.x, player.cursorTile.y);
					player.inventory.removeItem(player.inventory.hotbarSlot);
				}
			}
		}
	}

	if(keydown(KEY_LEFT)) player.cursor.x--;
	if(keydown(KEY_RIGHT)) player.cursor.x++;
	if(keydown(KEY_UP)) player.cursor.y--;
	if(keydown(KEY_DOWN)) player.cursor.y++;
	player.cursor.x = min(max(0, player.cursor.x), SCREEN_WIDTH - 1);
	player.cursor.y = min(max(0, player.cursor.y), SCREEN_HEIGHT - 1);

	if(keydown(KEY_MENU)) return false;

	player.physics(&player.props);

	return true;
}

int main(void)
{
	int menuSelect;
	extern bopti_image_t img_generating;
	extern font_t font_smalltext;
	int playerX, playerY;
	bool renderThisFrame = true;
	int w, h;
	int timer;

	save = (struct SaveData){
		.tileDataSize = WORLD_HEIGHT * WORLD_WIDTH * sizeof(Tile),
		.regionsX = WORLD_WIDTH / REGION_SIZE + 1,
		.regionsY = WORLD_HEIGHT / REGION_SIZE + 1,
		.tileData = (void*)RAM_START,
		.regionData = (unsigned char*)malloc(save.regionsX * save.regionsY),
		.error = -99,
	};

	memset(save.tileData, 0, WORLD_WIDTH * WORLD_HEIGHT * sizeof(Tile));

	if(!testRAM()) 
	{
		RAMErrorMenu();
		return 1;
	}

	srand(RTC_GetTicks());

	dgray_setdelays(923, 1742);
	dgray(DGRAY_ON);

	menuSelect = mainMenu();
	if(menuSelect == -1) return 1;
	
	player = (struct Player){
		.props = {0, 0, 0.0, 0.0, false, 12, 21},
		.health = 100,
		.cursor = {75, 32},
		.cursorTile = { 0 },
		.anim = { 0 },
		.inventory = {
			{{ 0 }},
			0,
			.getFirstFreeSlot = &getFirstFreeSlot,
			.removeItem = &removeItem,
			.stackItem = &stackItem
		},
		.physics = &handlePhysics
	};

	for(int slot = 0; slot < INVENTORY_SIZE; slot++) player.inventory.items[slot] = (Item){ITEM_NULL, 0};

	world.tiles = (Tile*)save.tileData;

	if(menuSelect == 0)
	{
		dclear(C_WHITE);
		dimage(31, 15, &img_generating);
		dupdate();
		generateWorld();
		memset(save.regionData, 1, save.regionsX * save.regionsY);
	} 
	else if(menuSelect == 1)
	{
		dgray(DGRAY_OFF);
		dclear(C_WHITE);
		dsize("Loading World...", NULL, &w, &h);
		dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Loading World...");
		dupdate();
		gint_switch(&loadSave);
		if(save.error != -99) 
		{
			loadFailMenu();
			return 1;
		}
		memset(save.regionData, 0, save.regionsX * save.regionsY);

		dgray(DGRAY_ON);
	}

	dfont(&font_smalltext);

	playerX = (WORLD_WIDTH / 2);
	playerY = 0;
	while(1)
	{
		if(world.tiles[playerY * WORLD_WIDTH + playerX].idx != 0)
		{
			playerY = (playerY - 3);
			break;
		}
		playerY++;
	}

	player.props.x = playerX << 3;
	player.props.y = playerY << 3;

	while(true)
	{
		timer = timer_setup(TIMER_ANY, (1000 / 60) * 1000, NULL);
		timer_start(timer);
		if(!update()) break;
		if(renderThisFrame)
		{
			render();
			renderThisFrame = false;
		}
		else renderThisFrame = true;
		timer_wait(timer);
	}

	dgray(DGRAY_OFF);
	dclear(C_WHITE);
	dfont(NULL);
	dsize("Saving World...", NULL, &w, &h);
	dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Saving World...");
	dupdate();
	gint_switch(&saveGame);
	if(save.error != -99) saveFailMenu();
	
	free(save.regionData);

	return 1;
}
