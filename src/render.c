#include <gint/gray.h>
#include <gint/defs/util.h>
#include <gint/std/stdio.h>

#include "render.h"

const unsigned int camMinX = SCREEN_WIDTH >> 1;
const unsigned int camMaxX = (MAP_WIDTH << 3) - (SCREEN_WIDTH >> 1);
const unsigned int camMinY = SCREEN_HEIGHT >> 1;
const unsigned int camMaxY = (MAP_HEIGHT << 3) - (SCREEN_HEIGHT >> 1);

void render(struct Map* map, struct Player* player)
{
	extern image_t img_player1;
	int camX = min(max(player->x, camMinX), camMaxX);
	int camY = min(max(player->y, camMinY), camMaxY);

//	Translating cam bounds to tile bounds is painful
	unsigned int tileLeftX = max(0, ((camX - (SCREEN_WIDTH >> 1)) >> 3) - 1);
	unsigned int tileRightX = min(MAP_WIDTH - 1, tileLeftX + (SCREEN_WIDTH >> 3) + 1);
	unsigned int tileTopY = max(0, ((camY - (SCREEN_HEIGHT >> 1)) >> 3) - 1);
	unsigned int tileBottomY = min(MAP_HEIGHT - 1, tileTopY + (SCREEN_HEIGHT >> 3) + 1);

	const Tile* currTile;
	unsigned int currTileX, currTileY;

	gclear(C_WHITE);

	for(unsigned int y = tileTopY; y <= tileBottomY; y++)
	{
		for(unsigned int x = tileLeftX; x <= tileRightX; x++)
		{
			currTile = map->tiles[y * MAP_WIDTH + x];
			currTileX = (x << 3) - (camX - (SCREEN_WIDTH >> 1));
			currTileY = (y << 3) - (camY - (SCREEN_HEIGHT >> 1));
			if(currTile->render) gimage(currTileX, currTileY, currTile->sprite);
		}
	}
	gimage(player->x - (camX - (SCREEN_WIDTH >> 1)), player->y - (camY - (SCREEN_HEIGHT >> 1)), &img_player1);

//	This is my entire render process debugger :D
	char buf[21];sprintf(buf,"%d %d %d %d",tileLeftX,tileRightX,tileTopY,tileBottomY);gtext(0,0,buf,C_BLACK,C_WHITE);

	gupdate();
}