#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/timer.h>
#include <stdbool.h>

#include "menu.h"
#include "syscalls.h"
#include "defs.h"
#include "save.h"
#include "inventory.h"
#include "entity.h"
#include "render.h"

int mainMenu()
{
	extern bopti_image_t img_mainmenu;
	extern bopti_image_t img_mainmenuselect;
	int selectPositions[] = {13, 30, 47};
	bool validSave = getSave();
	int selected = validSave ? 1 : 0;

	while(1)
	{
		dclear(C_WHITE);
		dimage(0, 0, &img_mainmenu);
		dimage(65, selectPositions[selected], &img_mainmenuselect);
		dupdate();

		clearevents();
		if(keydown(KEY_ALPHA)) gint_switch(takeVRAMCapture);
		if(keydown(KEY_MENU)) return -1;
		if(keydown(KEY_EXE)) 
		{
			if(selected == 2)
			{
				aboutMenu();
				continue;
			}
			else return selected;
		}

		if(keydown(KEY_UP)) selected--;
		if(keydown(KEY_DOWN)) selected++;
		if(selected == 1 && !validSave)
		{
			if(keydown(KEY_UP)) selected--;
			if(keydown(KEY_DOWN)) selected++;
		}
		selected = min(max(selected, 0), 2);

		while(keydown(KEY_UP) || keydown(KEY_DOWN)) clearevents();
	}
}

void inventoryMenuUpdate()
{
	extern bopti_image_t img_inventory, img_cursor;
	Item* item;
	int cursorSlotX, cursorSlotY;
	int cursorX = 64, cursorY = 32;
	Item held = {ITEM_NULL, 0};
	bool keypress = false;
	int freeSlot;
	int timer;
	int ticks = 0;
	int slot;

	while(true)
	{
		timer = timer_setup(TIMER_ANY, (1000 / 30) * 1000, NULL);
		timer_start(timer);
		clearevents();
		if(keydown(KEY_ALPHA)) gint_switch(&takeVRAMCapture);
		if(keydown(KEY_EXIT))
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
			return;
		}
		if(keydown(KEY_LEFT)) cursorX -= 2;
		if(keydown(KEY_RIGHT)) cursorX += 2;
		if(keydown(KEY_UP)) cursorY -= 2;
		if(keydown(KEY_DOWN)) cursorY += 2;
		cursorX = min(max(cursorX, 2), 125);
		cursorY = min(max(cursorY, 2), 48);

		if(!keypress && (keydown(KEY_F1) || keydown(KEY_F2)))
		{
			ticks = 0;
			cursorSlotX = cursorX / 16;
			cursorSlotY = cursorY / 17;
			slot = cursorSlotY * 8 + cursorSlotX;
			item = &player.inventory.items[slot];
			keypress = true;
			if(keydown(KEY_F1))
			{
				if(item->id == held.id)
				{
					player.inventory.stackItem(item, &held);
				}
				else
				{
					swap(*item, held);
				}
			}
			else if(keydown(KEY_F2))
			{
				if((item->id == held.id && held.number < items[held.id].maxStack) || held.id == ITEM_NULL)
				{
					player.inventory.stackItem(&held, &(Item){item->id, 1});
					player.inventory.removeItem(slot);
				}
			}
		}
		else if(keypress)
		{
			ticks++;
			if(keydown(KEY_F2))
			{
				if(ticks % 5 == 0) keypress = false;
			}
			else
			{
				if(!keydown(KEY_F1)) keypress = false;
			}
		}
		if(keydown(KEY_DEL))
		{
			cursorSlotX = cursorX / 16;
			cursorSlotY = cursorY / 17;
			slot = cursorSlotY * 8 + cursorSlotX;
			player.inventory.items[cursorSlotX] = (Item){ITEM_NULL, 0};
		}
		
	//	Render
		dimage(0, 0, &img_inventory);
		for(int slot = 0; slot < INVENTORY_SIZE; slot++)
		{
			item = &player.inventory.items[slot];
			if(item->id != ITEM_NULL)
			{
				renderItem((slot % 8) * 16 + 1, (slot / 8) * 17 + 1, *item);
			}
		}
		if(held.id != ITEM_NULL)
		{
			renderItem(cursorX - 7, min(35, cursorY - 7), held);
		}
		dimage(cursorX - 2, cursorY - 2, &img_cursor);
		dupdate();
		timer_wait(timer);
		ticks++;
	}
}

void RAMErrorMenu()
{
	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "RAM test failed!");
	dtext(0, 8, C_BLACK, "Please report bug.");
	dtext(0, 24, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		clearevents();
		if(keydown(KEY_EXIT)) return;
	}
}

void loadFailMenu()
{
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
		clearevents();
		if(keydown(KEY_EXIT)) return;
	}
}

void saveFailMenu()
{
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
		clearevents();
		if(keydown(KEY_EXIT)) return;
	}
}

void aboutMenu()
{
	extern bopti_image_t img_confetti;
	const char* text[] = {
		"Terrario by KBD2",
		" ",
		"World:",
		"4,6,8:Move",
		"Arrows:Cursor",
		"7,9:Mine/Place",
		"F1,F2,F3:Hotbar",
		"[SHIFT]:Inventory",
		"[MENU]:Exit",
		" ",
		"Inventory:",
		"Arrows:Cursor",
		"F1:Take/Place Stack",
		"F2:Take Single",
		"[DEL]:Delete",
		"[EXIT]:Exit",
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
	int ticks;
	float scroll;

	while(y < lines * 13 + 25)
	{
		ticks = RTC_GetTicks();
		dclear(C_WHITE);
		for(int line = 0; line < lines; line++)
		{
			dsize(text[line], NULL, &width, &height);
			dtext(64 - (width / 2), 64 - y + line * (height + 6), C_BLACK, text[line]);
		}
		dimage(1, 74 - y + (lines - 1) * (height + 6), &img_confetti);
		dupdate();

		clearevents();
		if(keydown(KEY_EXIT)) return;
		if(keydown(KEY_UP))
		{
			scroll = 0;
		}
		else if(keydown(KEY_DOWN))
		{
			scroll = 3;
		}
		else scroll = 0.5;
		y += scroll;
		while(!RTC_Elapsed_ms(ticks, 50)){}
	}

	ticks = RTC_GetTicks();
	while(!RTC_Elapsed_ms(ticks, 3000)){}
}