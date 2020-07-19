#include <gint/gray.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <stdbool.h>

#include "menu.h"
#include "syscalls.h"
#include "defs.h"
#include "save.h"

int mainMenu()
{
	extern bopti_image_t img_mainmenu;
	extern bopti_image_t img_mainmenuselect;
	int selectPositions[] = {13, 30, 47};
	int selected = 0;
	bool validSave = getSave();

	while(1)
	{
		dclear(C_WHITE);
		dimage(0, 0, &img_mainmenu);
		dimage(65, selectPositions[selected], &img_mainmenuselect);
		dupdate();

		clearevents();
		if(keydown(KEY_MENU)) RebootOS();
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
		if(keydown(KEY_EXIT)) RebootOS();
	}
}

void loadFailMenu()
{
	dclear(C_WHITE);
	dtext(0, 0, C_BLACK, "Failed to load");
	dprint(0, 8, C_BLACK, "\\TERRARIO\\reg%d.dat", save.error);
	dtext(0, 16, C_BLACK, "Please report bug or");
	dtext(0, 24, C_BLACK, "delete \\TERRARIO.");
	dtext(0, 40, C_BLACK, "[EXIT] to exit");
	dupdate();

	while(1)
	{
		clearevents();
		if(keydown(KEY_EXIT)) RebootOS();
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
		if(keydown(KEY_EXIT)) RebootOS();
	}
}

void aboutMenu()
{
	extern bopti_image_t img_confetti;
	const int lines = 20;
	const char* text[] = {
		"Terrario by KBD2",
		" ",
		"Controls:",
		"4,6,8:Move",
		"Arrows:Cursor",
		"7,9:Mine/Place",
		"F1,F2,F3:Hotbar",
		"[MENU]:Exit",
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