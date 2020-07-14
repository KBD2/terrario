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
	int versionWidth;
	int selectPositions[] = {26, 44};
	int selected = 0;
	bool validSave = getSave();

	dsize(VERSION, NULL, &versionWidth, NULL);

	while(1)
	{
		dimage(0, 0, &img_mainmenu);
		dimage(65, selectPositions[selected], &img_mainmenuselect);
		dtext(SCREEN_WIDTH - versionWidth, 0, C_BLACK, VERSION);
		dupdate();

		clearevents();
		if(keydown(KEY_MENU)) RebootOS();
		if(keydown(KEY_UP)) selected--;
		if(keydown(KEY_DOWN)) selected++;
		selected = min(max(selected, 0), validSave);
		if(keydown(KEY_EXE)) return selected;
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