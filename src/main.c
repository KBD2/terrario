#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/gray.h>
#include <gint/std/stdio.h>

#include "map.h"

int main(void)
{
	struct Map map;
	char buffer[21];

	generateMap(&map);

	gray_start();

	sprintf(buffer, "%d %d", map.tiles[0].sprite, tiles[0].sprite);
	gtext(0, 0, buffer, C_BLACK, C_WHITE);
	gupdate();

	getkey();
	gray_stop();

	return 1;
}
