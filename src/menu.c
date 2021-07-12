// Miscellaneous menus

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/timer.h>
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

struct {
	char year[5];
	char month[3];
	char day[3];
	char hour[3];
	char minute[3];
} timestamp;

const char *title_messages[] = {
	"Debug edition!",
	"Shut up and Dig Gaiden!",
	"Sand is Overpowered",
	"A Bunnies Tale",
	"There is No Cow Layer",
	"Purple Grass!",
	"Dig Peon, Dig!",
	"Epic Dirt",
	"Also try Casio Kart!", // Yeah sorry if you don't like it, it's a reference to my own Casio Calculator game...
	"Terrario: Terrario:",
	"Press Shift+AC", // Love this one
	"Dividing by zero",
	"FA-124? What's That?",
	"Now with 4 COLORS",
	"A good day to dig hard",
	"Now in a calc near you!",
	"You sand bro?",
	"Legend of Maxx",
	"NOT THE BEES!!!",
	"Oficially Duck-free!",
	"9 + 1 = 11",
	"Hey Guys!",
	"Totally not Terraria!"
};

int mainMenu()
{
	extern bopti_image_t img_mainmenu, img_ui_mainmenuselect, img_ents_slime, img_sunmoon;
	int selectPositions[] = {13, 30, 47};
	bool validSave = getSave();
	int selected = validSave ? 1 : 0;
	key_event_t key;
	volatile int flag = 0;
	int timer;
	unsigned int frames = 300;
	int bunnyBlink = 0;

	int orbX, orbY;
	float dayPolarAngle;

	int message_index = (rand() % 22) + 1;
	int message_x = 256;

#ifdef DEBUGMODE
	message_index = 0;
#endif

	timer = timer_configure(TIMER_ANY, (1000 / 30) * 1000, GINT_CALL(&frameCallback, &flag));
	timer_start(timer);

	while(1)
	{
		dclear(C_WHITE);

		dayPolarAngle = (((float)PI * 2.0) / (float)1200) * (float)frames;
//		Sun
		orbX = 56 * cos(dayPolarAngle + PI / 2.0) + 56;
		orbY = 64 * sin(dayPolarAngle + PI / 2.0) + 64;
		dsubimage(orbX, orbY, &img_sunmoon, 0, 0, 16, 16, DIMAGE_NONE);
//		Moon
		orbX = 56 * cos(dayPolarAngle - PI / 2.0) + 56;
		orbY = 64 * sin(dayPolarAngle - PI / 2.0) + 64;
		dsubimage(orbX, orbY, &img_sunmoon, 16, 0, 16, 16, DIMAGE_NONE);

		dimage(0, 0, &img_mainmenu);
		dimage(65, selectPositions[selected], &img_ui_mainmenuselect);
		dsubimage(43, 52, &img_ents_slime, 0, (frames & 16) ? 14 : 1, 16, 12, DIMAGE_NONE);
		if(bunnyBlink == 0)
		{
			if(rand() % 30 == 0) bunnyBlink = 3;
		}
		else
		{
			dline(91, 57, 91, 58, C_WHITE);
			bunnyBlink--;
		}
		dprint_opt(message_x / 2, 56, C_BLACK, C_WHITE, DTEXT_LEFT, DTEXT_TOP, "Terrario: %s", title_messages[message_index]);
		dupdate();

		message_x--;
		if (message_x < -256) message_x = 256;

		key = pollevent();
		while(key.type != KEYEV_NONE)
		{
			switch(key.key)
			{
				case KEY_OPTN:
					if(key.type == KEYEV_DOWN) gint_world_switch(GINT_CALL(&takeVRAMCapture));
					break;

				case KEY_MENU:
					timer_stop(timer);
					return -1;
				
				case KEY_EXE:
					if(selected == 2)
					{
						aboutMenu();
						break;
					}
					else
					{
						timer_stop(timer);
						return selected;
					}
				
				case KEY_UP:
					if(key.type != KEYEV_DOWN) break;
					selected--;
					if(selected == 1 && !validSave) selected--;
					break;

				case KEY_DOWN:
					if(key.type != KEYEV_DOWN) break;
					selected++;
					if(selected == 1 && !validSave) selected++;
					break;

				default:
					break;
			}
			key = pollevent();
		}

		selected = min(max(selected, 0), 2);

		while(!flag) sleep();
		flag = 0;
		frames++;
	}
}

int exitMenu()
{
	int width;
	extern bopti_image_t img_ui_quit;
	key_event_t key;

	while(keydown(KEY_MENU)) clearevents();
	drect_border(20, 9, 106, 54, C_WHITE, 1, C_BLACK);
	dsize("Quit?", NULL, &width, NULL);
	dtext(64 - width / 2, 11, C_BLACK, "Quit?");
	dtext(22, 42, C_BLACK, "MENU: Yes");
	dsize("EXIT: No", NULL, &width, NULL);
	dtext(105 - width, 42, C_BLACK, "EXIT: No");
	dtext_opt(64, 48, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, "AC/ON: Don't save");
	dimage(47, 17, &img_ui_quit);
	dupdate();

	while(1)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		switch(key.key)
		{
			case KEY_MENU:
				return 1;
			
			case KEY_EXIT:
				return 0;
			
			case KEY_ACON:
				return 2;
			
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
	if(save.error == -2) dtext(0, 8, C_BLACK, "\\TERRARIO\\housing.dat");
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

void itemMenu()
{
	key_event_t key;
	extern bopti_image_t img_ui_slots;
	Item item = {ITEM_STONE, 1};
	int slot;

	while(1)
	{
		dclear(C_WHITE);
		dsubimage(0, 0, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NONE);
		renderItem(1, 1, &item);
		dtext(0, 17, C_BLACK, items[item.id].name);
		dtext_opt(127, 0, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F1: Day");
		dtext_opt(127, 6, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F2: Night");
		dtext_opt(127, 12, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F3: Heal");
		dtext_opt(127, 18, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F4: Butcher");
		dtext_opt(127, 24, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F5: Save");
		dtext_opt(127, 30, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "F6: Spawn");
		dtext_opt(127, 36, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "SHIFT: Force housing");
		dupdate();

		key = getkey_opt(GETKEY_REP_ARROWS, NULL);
		switch(key.key)
		{
			case KEY_EXIT:
				return;
			
			case KEY_EXE:
				while(item.amount > 0)
				{
					slot = player.inventory.getFirstFreeSlot(item.id);
					if(slot > -1) player.inventory.stackItem(&player.inventory.items[slot], &item);
					else return;
				}
				return;
			
			case KEY_UP:
				item.amount++;
				if(item.amount > items[item.id].maxStack) item.amount = 1;
				break;
			
			case KEY_DOWN:
				item.amount--;
				if(item.amount == 0) item.amount = items[item.id].maxStack;
				break;
			 
			 case KEY_LEFT:
			 	if(item.id == ITEM_STONE) item.id = ITEMS_COUNT - 1;
				else item.id--;
				if(item.amount > items[item.id].maxStack) item.amount = items[item.id].maxStack;
				break;
			
			case KEY_RIGHT:
				if(item.id == ITEMS_COUNT - 1) item.id = ITEM_STONE;
				else item.id++;
				if(item.amount > items[item.id].maxStack) item.amount = items[item.id].maxStack;
				break;
			
			case KEY_F1:
				world.timeTicks = timeToTicks(4, 30);
				return;
			
			case KEY_F2:
				world.timeTicks = timeToTicks(19, 30);
				return;
			
			case KEY_F3:
				player.combat.health = player.maxHealth;
				return;
			
			case KEY_F4:
				for(int idx = 0; idx < MAX_ENTITIES; idx++) world.entities[idx].id = -1;
				return;
			
			case KEY_F5:
				gint_world_switch(GINT_CALL(&saveGame));
				return;
			
			case KEY_F6:
				player.props.x = player.spawn.x;
				player.props.y = player.spawn.y;
				return;
			
			case KEY_SHIFT:
				doMarkerChecks();
				doNPCHouseCheck();
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
	const char *thanksText[] = {
		"Special thanks to:",
		"Lephenixnoir",
		"Dark Storm",
		"Yatis",
		"and you!"
	};
	const char *versionText[] = {
		VERSION,
		"gint " GINT_VERSION,
		buffer
	};
	const char *controlsText[] = {
		"World:",
		"4,6,8: Move",
		"Arrows: Cursor",
		"7: Use Held Item",
		"9: Interact",
		"F1,F2,F3: Hotbar",
		"[SHIFT]: Inventory",
		"[ALPHA]: Crafting",
		"[MENU]: Exit",
		" ",
		"Inventory:",
		"Arrows: Cursor",
		"F1: Take/Place Stack",
		"F2: Take Single",
		"[DEL]: Delete",
		"[SHIFT]: Exit",
		"[ALPHA]: Crafting",
		"F4: Chest Tab",
		"F5: Inventory Tab",
		"F6: Equipment Tab",
		" ",
		"Crafting:",
		"Left/Right: Scroll",
		"[EXE]: Craft",
		"[SHIFT]: Inventory",
		"[ALPHA]: Exit"
	};
	const int thanksLines = sizeof(thanksText) / sizeof(char*);
	const int versionLines = sizeof(versionText) / sizeof(char*);
	const int controlLines = sizeof(controlsText) / sizeof(char*);
	int scroll = 0;
	bool ingredients = false;
	int ingredientsScroll = 0;
	extern bopti_image_t img_confetti, img_ui_arrowshoriz, img_ui_arrowsall, img_ui_abouttabs, img_ui_slot_highlight, img_ui_slots;
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
				for(int line = 0; line < thanksLines; line++)
				{
					dtext_opt(64, 7 * line + 1, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, thanksText[line]);
				}
				for(int line = 0; line < versionLines; line++)
				{
					dtext_opt(64, 7 * line + 37, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, versionText[line]);
				}
				dline(48, 35, 80, 35, C_LIGHT);
				break;
			
			case MENU_CONTROLS:
				dimage(122, 55, &img_ui_arrowshoriz);
				for(int line = 0; line < 8; line++)
				{
					if(line + scroll == controlLines) break;
					dtext_opt(64, line * 7, C_BLACK, C_WHITE, DTEXT_MIDDLE, DTEXT_TOP, controlsText[line + scroll]);
				}
				break;
			
			case MENU_CRAFTING:
				dimage(112, 54, &img_ui_arrowsall);
				for(int dR = 0; dR < 8 && scroll + dR < numRecipes; dR++)
				{
					dsubimage(17 * dR, 0, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NONE);
					renderItem(17 * dR + 1, 1, (Item *)&recipes[scroll + dR].result);
				}
				if(recipes[scroll].required == TILE_NULL) workbench = "anywhere";
				else workbench = tiles[recipes[scroll].required].name;
				dprint(0, 18, C_BLACK, "%s at %s", items[recipes[scroll].result.id].name, workbench);
				for(int dI = 0; dI < recipes[scroll].numIngredients; dI++)
				{
					dsubimage(17 * dI, 24, &img_ui_slots, 0, 0, 16, 17, DIMAGE_NONE);
					renderItem(17 * dI + 1, 25, (Item *)&recipes[scroll].ingredients[dI]);
				}
				if(!ingredients) dimage(0, 0, &img_ui_slot_highlight);
				else
				{
					dimage(17 * ingredientsScroll, 24, &img_ui_slot_highlight);
					dtext(0, 42, C_BLACK, items[recipes[scroll].ingredients[ingredientsScroll].id].name);
				}
				break;
		}
		dimage(0, 57, &img_ui_abouttabs);
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

void debugNumberMenu(int *numbers, int num)
{
	dclear(C_WHITE);
	for(int i = 0; i < num; i++)
	{
		dprint(0, 8 * i, C_BLACK, "0x%X", numbers[i]);
	}
	dupdate();

	getkey_opt(GETKEY_NONE, NULL);
}
