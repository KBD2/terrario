#include <gint/gray.h>
#include <gint/gint.h>
#include <stdio.h>
#include <gint/std/stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gint/keyboard.h>

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

struct SaveData save;
struct World world;

// Checks if the RAM we'll be using to store world tiles works
bool testRAM()
{
	unsigned int* RAMAddress = (void*)RAM_START;
	unsigned int* RAMTestAddress = (void*)0x88000000;
	*RAMAddress = 0xC0FFEE;
	if(*RAMAddress == 0xC0FFEE && *RAMAddress != *RAMTestAddress) return true;
	return false;
}

int main(void)
{
	struct Player player = {
		{0, 0, 0.0, 0.0, false, 12, 21},
		100,
		{0, 0},
		{0, 0},
		0, 0, 0,
		&updatePlayer,
		&handlePhysics
	};
	int ticks;
	int menuSelect;
	extern bopti_image_t img_generating;

	save.tileDataSize = WORLD_HEIGHT * WORLD_WIDTH * sizeof(Tile);
	save.regionsX = WORLD_WIDTH / REGION_SIZE + 1;
	save.regionsY = WORLD_HEIGHT / REGION_SIZE + 1;
	save.tileData = (void*)RAM_START;
	save.regionData = save.tileData + save.tileDataSize;
	save.error = -1;

	memset(save.tileData, 0, WORLD_WIDTH * WORLD_HEIGHT * 2);

	if(!testRAM()) RAMErrorMenu();

	srand(RTC_GetTicks());

	dgray_setdelays(923, 1742);
	dgray(DGRAY_ON);

	menuSelect = mainMenu();

	world.tiles = (Tile*)save.tileData;

	if(menuSelect == 0)
	{
		memset(save.regionData, 1, save.regionsX * save.regionsY);
		dclear(C_WHITE);
		dimage(31, 15, &img_generating);
		dupdate();
		generateWorld();
	} 
	else if(menuSelect == 1)
	{
		dgray(DGRAY_OFF);
		dclear(C_WHITE);
		dtext(0, 0, C_BLACK, "Loading world...");
		dupdate();
		gint_switch(&loadSave);
		if(save.error > -1) loadFailMenu();
		memset(save.regionData, 0, save.regionsX * save.regionsY);

		dgray(DGRAY_ON);
	}
	

	while(1)
	{
		ticks = RTC_GetTicks();
		player.update(&player);
		render(&player);
		dprint(123, 0, C_BLACK, "%d", RTC_GetTicks() - ticks);
		if(keydown(KEY_SHIFT)) gint_switch(&takeVRAMCapture);
		dupdate();
//		30FPS
		while(!RTC_Elapsed_ms(ticks, 33)){}
	}

	return 1;
}
