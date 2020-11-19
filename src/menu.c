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
#include <gint/std/stdio.h>

#include "menu.h"
#include "syscalls.h"
#include "defs.h"
#include "save.h"
#include "inventory.h"
#include "entity.h"
#include "render.h"
#include "world.h"
#include "crafting.h"

struct {
	char year[5];
	char month[3];
	char day[3];
	char hour[3];
	char minute[3];
} timestamp;

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
		#ifdef DEBUGMODE
		dtext_opt(0, 0, C_BLACK, C_WHITE, DTEXT_LEFT, DTEXT_TOP, "DEBUG BUILD");
		#endif
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

	memset(save.tileData, 0, game.WORLD_WIDTH * game.WORLD_HEIGHT);

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
	dtext(105 - width, 45, C_BLACK, "EXIT: No");
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
	if(save.error > -1) dprint(0, 8, C_BLACK, "\\TERRARIO\\reg%i-%i.dat", save.error >> 4, save.error & 0xf);
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
	dprint(0, 8, C_BLACK, "\\TERRARIO\\reg%i-%i.dat", save.error >> 4, save.error & 0xf);
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

	dfont(NULL);

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Mem alloc returned");
	dtext(0, 8, C_BLACK, "NULL! Please report");
	dtext(0, 16, C_BLACK, "this bug.");
	dtext(0, 32, C_BLACK, "[EXIT] to quit");
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				while(true) gint_osmenu();
			
			default:
				break;
		}
	}
}

void incompatibleMenu(int code)
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Sorry, this game is");
	dtext(0, 8, C_BLACK, "not compatible with");
	dtext(0, 16, C_BLACK, "your calculator :(");
	dprint(0, 32, C_BLACK, "HW code: %d", code);
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

enum MenuTabs {
	MENU_ABOUT,
	MENU_CONTROLS,
	MENU_CRAFTING
};

void aboutMenu()
{
	const char *months[12] = {
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec"
	};
	char buffer[64];
	memset(&timestamp, 0, sizeof(timestamp));
//	Offset 0x3C into the addin file is the start of the build timestamp.
//	0x00300000 in memory is the start of the mapped addin in ROM.
	memcpy(timestamp.year, (char *)0x0030003C, 4);
	memcpy(timestamp.month, (char *)0x00300041, 2);
	memcpy(timestamp.day, (char *)0x00300043, 2);
	memcpy(timestamp.hour, (char *)0x00300046, 2);
	memcpy(timestamp.minute, (char *)0x00300048, 2);
	const char *month = months[atoi(timestamp.month) - 1];
	sprintf(buffer, "%s %s %s %s:%s", timestamp.day, month, timestamp.year, timestamp.hour, timestamp.minute);
	const char *aboutText[] = {
		"Special thanks to:",
		"Lephenixnoir - Gint",
		"Memallox - Newlib",
		"Dark Storm",
		"Yatis",
		"",
		VERSION,
		buffer
	};
	const char *controlsText[] = {
		"World:",
		"4,6,8:Move",
		"Arrows:Cursor",
		"7:Use Held Item",
		"9:Interact",
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
		"F5:Inventory Tab",
		"F6:Chest Tab",
		" ",
		"Crafting:",
		"Left/Right:Scroll",
		"[EXE]:Craft",
		"[SHIFT]:Inventory",
		"[ALPHA]:Exit"
	};
	const int aboutLines = sizeof(aboutText) / sizeof(char*);
	const int controlLines = sizeof(controlsText) / sizeof(char*);
	int scroll = 0;
	bool ingredients = false;
	int ingredientsScroll = 0;
	extern bopti_image_t img_confetti, img_arrowshoriz, img_arrowsall, img_abouttabs, img_slot_highlight, img_slot;
	key_event_t key;
	extern font_t font_smalltext;
	enum MenuTabs menu = MENU_ABOUT;
	char *workbench;

	dfont(&font_smalltext);

	while(true)
	{
		dclear(C_WHITE);
		switch(menu)
		{
			case MENU_ABOUT:
				dimage(89, 36, &img_confetti);
				for(int line = 0; line < aboutLines; line++)
				{
					dtext_opt(64, 7 * line, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, aboutText[line]);
				}
				break;
			
			case MENU_CONTROLS:
				dimage(122, 55, &img_arrowshoriz);
				for(int line = 0; line < 8; line++)
				{
					if(line + scroll == controlLines) break;
					dtext_opt(64, line * 7, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, controlsText[line + scroll]);
				}
				break;
			
			case MENU_CRAFTING:
				dimage(112, 54, &img_arrowsall);
				for(int dR = 0; dR < 8 && scroll + dR < numRecipes; dR++)
				{
					dimage(17 * dR, 0, &img_slot);
					renderItem(17 * dR + 1, 1, (Item *)&recipes[scroll + dR].result);
				}
				if(recipes[scroll].required == TILE_NULL) workbench = "anywhere";
				else workbench = tiles[recipes[scroll].required].name;
				dprint(0, 18, C_BLACK, "%s at %s", items[recipes[scroll].result.id].name, workbench);
				for(int dI = 0; dI < recipes[scroll].numIngredients; dI++)
				{
					dimage(17 * dI, 24, &img_slot);
					renderItem(17 * dI + 1, 25, (Item *)&recipes[scroll].ingredients[dI]);
				}
				if(!ingredients) dimage(0, 0, &img_slot_highlight);
				else
				{
					dimage(17 * ingredientsScroll, 24, &img_slot_highlight);
					dtext(0, 42, C_BLACK, items[recipes[scroll].ingredients[ingredientsScroll].id].name);
				}
				break;
		}
		dimage(0, 57, &img_abouttabs);
		dupdate();

		key = getkey_opt(GETKEY_REP_ARROWS, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				dfont(NULL);
				return;
			
			case KEY_F1:
				menu = MENU_ABOUT;
				break;
			
			case KEY_F2:
				scroll = 0;
				menu = MENU_CONTROLS;
				break;

			case KEY_F3:
				scroll = 0;
				ingredients = false;
				ingredientsScroll = 0;
				menu = MENU_CRAFTING;
				break;
			
			case KEY_UP:
				if(menu == MENU_CONTROLS)
				{
					if(scroll == 0) break;
					scroll--;
				}
				else if(menu == MENU_CRAFTING)
				{
					if(!ingredients) break;
					ingredients = false;
				}
				break;
			
			case KEY_DOWN:
				if(menu == MENU_CONTROLS)
				{
					if(scroll == controlLines - 8) break;
					scroll++;
				}
				else if(menu == MENU_CRAFTING)
				{
					if(ingredients) break;
					ingredients = true;
					ingredientsScroll = 0;
				}
				break;
			
			case KEY_LEFT:
				if(menu == MENU_CRAFTING)
				{
					if((!ingredients && scroll == 0) || (ingredients && ingredientsScroll == 0)) break;
					if(ingredients) ingredientsScroll--;
					else scroll--;
				}
				break;
			
			case KEY_RIGHT:
				if(menu == MENU_CRAFTING)
				{
					if((!ingredients && scroll == numRecipes - 1) || (ingredients && ingredientsScroll == recipes[scroll].numIngredients - 1)) break;
					if(ingredients) ingredientsScroll++;
					else scroll++;
				}
				break;
		}
	}
}

void lowSpaceMenu(int mediaFree)
{
	key_event_t key;

	dclear(C_WHITE);
	dprint(0, 0, C_BLACK, "Only %.1jkB", (int)((float)mediaFree/100));
	dtext(0, 8, C_BLACK, "of SMEM free!");
	dtext(0, 16, C_BLACK, "Please ensure");
	dtext(0, 24, C_BLACK, "350kB free.");
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

void saveVersionDifferenceMenu(char *saveVersion)
{
	key_event_t key;

	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Save version:");
	dtext(0, 8, C_BLACK, saveVersion);
	dtext(0, 16, C_BLACK, "Game version:");
	dtext(0, 24, C_BLACK, VERSION);
	dtext(0, 32, C_BLACK, "Save may not load.");
	dtext(0, 48, C_BLACK, "[EXIT] to continue");
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