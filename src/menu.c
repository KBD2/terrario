// Miscellaneous menus

#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/timer.h>
#include <stdbool.h>
#include <gint/std/string.h>
#include <gint/std/stdlib.h>
#include <gint/clock.h>

#include "menu.h"
#include "syscalls.h"
#include "defs.h"
#include "save.h"
#include "inventory.h"
#include "entity.h"
#include "render.h"
#include "world.h"
#include "crafting.h"

int mainMenu()
{
	extern bopti_image_t img_mainmenu;
	extern bopti_image_t img_mainmenuselect;
	int selectPositions[] = {13, 30, 47};
	bool validSave = getSave();
	int selected = validSave ? 1 : 0;
	key_event_t key;

	while(1)
	{
		dclear(C_WHITE);
		dimage(0, 0, &img_mainmenu);
		dimage(65, selectPositions[selected], &img_mainmenuselect);
		dupdate();

		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_switch(&takeVRAMCapture);
				break;

			case KEY_MENU:
				return -1;
			
			case KEY_EXE:
				if(selected == 2)
				{
					aboutMenu();
					break;
				}
				else return selected;
			
			case KEY_UP:
				selected--;
				if(selected == 1 && !validSave) selected--;
				break;
			case KEY_DOWN:
				selected++;
				if(selected == 1 && !validSave) selected++;
				break;
			
			case KEY_ACON:
				debugMenu();
				break;
			
			default:
				break;
		}

		selected = min(max(selected, 0), 2);
	}
}

void debugMenu()
{
	int selected = 0;
	int which = 0;
	key_event_t key;

	memset(save.tileData, 0, WORLD_WIDTH * WORLD_HEIGHT);

	while(true)
	{
		dclear(C_WHITE);
		if(which == 0)
		{
			dimage(0, 9, tiles[selected].sprite);
			if(tiles[selected].spriteType == TYPE_SHEET || tiles[selected].spriteType == TYPE_SHEET_VAR)
			{
				for(int y = 0; y < 5; y++) dhline(y * 9 + 9, C_WHITE);
			}
			for(int variant = 0; variant < 3; variant++)
			{
				for(int x = 0; x < 5; x++) dvline(x * 9 + variant * 37, C_WHITE);
			}
			dprint(0, 0, C_BLACK, "Tile %s", tiles[selected].name);
		}
		else
		{
			dprint(0, 0, C_BLACK, "Item %s", items[selected].name);
			dimage(0, 8, items[selected].sprite);
		}
		dupdate();
		while(true)
		{
			key = getkey_opt(GETKEY_NONE, NULL);
			if(key.type == KEYEV_DOWN) break;
		}
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			case KEY_LEFT:
				selected--;
				if(which == 0 && selected < 0) selected = TILES_COUNT - 1;
				if(which == 1 && selected < 0) selected = ITEMS_COUNT - 1;
				break;
			case KEY_RIGHT:
				selected++;
				if(which == 0 && selected == TILES_COUNT) selected = 0;
				if(which == 1 && selected == ITEMS_COUNT) selected = 0;
				break;
			
			case KEY_SHIFT:
				which = !which;
				selected = 0;
				break;
			
			default:
				break;
		}
	}
}

bool exitMenu()
{
	int width;
	extern bopti_image_t img_quit;
	key_event_t key;

	while(keydown(KEY_MENU)) clearevents();
	drect_border(19, 9, 107, 52, C_WHITE, 1, C_BLACK);
	dsize("Quit?", NULL, &width, NULL);
	dtext(64 - width / 2, 12, C_BLACK, "Quit?");
	dtext(22, 45, C_BLACK, "MENU: Yes");
	dsize("EXIT: No", NULL, &width, NULL);
	dtext(109 - width, 45, C_BLACK, "EXIT: No");
	dimage(47, 19, &img_quit);
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_MENU:
				return true;
			
			case KEY_EXIT:
				return false;
			
			case KEY_ACON:
				RebootOS(); // Don't save/optimize
			
			default:
				break;
		}
	}
}

void RAMErrorMenu()
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "RAM test failed!");
	dtext(0, 8, C_BLACK, "Please report bug.");
	dtext(0, 24, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			default:
				break;
		}
	}
}

void loadFailMenu()
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Failed to load");
	if(save.error > -1) dprint(0, 8, C_BLACK, "\\TERRARIO\\reg%d.dat", save.error);
	if(save.error == -1) dtext(0, 8, C_BLACK, "\\TERRARIO\\player.dat");
	dtext(0, 16, C_BLACK, "Please report bug or");
	dtext(0, 24, C_BLACK, "delete \\TERRARIO.");
	dtext(0, 40, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			default:
				break;
		}
	}
}

void saveFailMenu()
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Failed to write");
	dprint(0, 8, C_BLACK, "\\TERRARIO\\reg%d.dat", save.error);
	dtext(0, 16, C_BLACK, "Please optimise SMEM");
	dtext(0, 24, C_BLACK, "and ensure 250kB free");
	dtext(0, 32, C_BLACK, "if this is a new save");
	dtext(0, 48, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			default:
				break;
		}
	}
}

void memoryErrorMenu()
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "malloc() returned");
	dtext(0, 8, C_BLACK, "NULL! Please report");
	dtext(0, 16, C_BLACK, "this bug.");
	dtext(0, 32, C_BLACK, "[EXIT] to reboot");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				RebootOS();;
			
			default:
				break;
		}
	}
}

void aboutMenu()
{
	extern bopti_image_t img_confetti;
	const char *text[] = {
		"Terrario by KBD2",
		" ",
		"World:",
		"4,6,8:Move",
		"Arrows:Cursor",
		"7,9:Mine/Place",
		"7:Attack",
		"F1,F2,F3:Hotbar",
		"[SHIFT]:Inventory",
		"[ALPHA]:Crafting",
		"[MENU]:Exit",
		" ",
		"Inventory:",
		"Arrows:Cursor",
		"F1:Take/Place Stack",
		"F2:Take Single",
		"[DEL]:Delete",
		"[SHIFT]:Exit",
		"[ALPHA]:Crafting",
		" ",
		"Crafting:",
		"Left/Right:Scroll",
		"[EXE]:Craft",
		"[SHIFT]:Inventory",
		"[ALPHA]:Exit",
		" ",
		"Special thanks to:",
		"Re-Logic - Terraria",
		"Lephenixnoir - Gint",
		"Memallox - Newlib",
		"Dark Storm",
		" ",
		VERSION,
		__DATE__,
		" ",
		" ",
		"Enjoy!"
	};
	const int lines = sizeof(text) / sizeof(char*);
	float y = 0;
	int width, height;
	int timer;
	float scroll;
	key_event_t key;
	int flag = 0;

	timer = timer_setup(TIMER_ANY, 50 * 1000, &frameCallback, &flag);
	timer_start(timer);
	while(y < lines * 13 + 25)
	{
		
		dclear(C_WHITE);
		for(int line = 0; line < lines; line++)
		{
			dsize(text[line], NULL, &width, &height);
			dtext(64 - (width / 2), 64 - y + line * (height + 6), C_BLACK, text[line]);
		}
		dimage(1, 74 - y + (lines - 1) * (height + 6), &img_confetti);
		dupdate();

		scroll = 0.5;

		key = pollevent();
		while(key.type != KEYEV_NONE) key = pollevent();
		if(keydown(KEY_EXIT)) return;
		if(keydown(KEY_UP)) scroll = 0;
		if(keydown(KEY_DOWN)) scroll = 3;
		y += scroll;

		while(!flag) sleep();
		flag = 0;
	}
	timer_stop(timer);

	timer = timer_setup(TIMER_ANY, 3000 * 1000, NULL);
	timer_start(timer);
	timer_wait(timer);
}

void lowSpaceMenu(int mediaFree)
{
	key_event_t key;

	dclear(C_WHITE);
	dprint(0, 0, C_BLACK, "Only %.1jkB", (int)((float)mediaFree/100));
	dtext(0, 8, C_BLACK, "of SMEM free!");
	dtext(0, 16, C_BLACK, "Please optimise SMEM");
	dtext(0, 24, C_BLACK, "and ensure 300kB free");
	dtext(0, 40, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			default:
				break;
		}
	}
}