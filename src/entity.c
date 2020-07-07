#include <gint/keyboard.h>
#include <gint/defs/util.h>

#include <gint/gray.h>
#include <gint/std/stdio.h>

#include "syscalls.h"
#include "entity.h"
#include "map.h"
#include "defs.h"

void updatePlayer(struct Map* map, struct Player* self)
{
	clearevents();

	if(keydown(KEY_4)) self->props.xVel = -3;
	if(keydown(KEY_6)) self->props.xVel = 3;
	if(keydown(KEY_8) && self->props.touchingTileTop) self->props.yVel = -5;

	if(keydown(KEY_7)) map->tiles[self->cursorTile.y * MAP_WIDTH + self->cursorTile.x] = &tiles[TILE_NOTHING];
	if(keydown(KEY_9)) map->tiles[self->cursorTile.y * MAP_WIDTH + self->cursorTile.x] = &tiles[TILE_STONE];

	if(keydown(KEY_LEFT)) self->cursor.x--;
	if(keydown(KEY_RIGHT)) self->cursor.x++;
	if(keydown(KEY_UP)) self->cursor.y--;
	if(keydown(KEY_DOWN)) self->cursor.y++;
	self->cursor.x = min(max(0, self->cursor.x), SCREEN_WIDTH - 1);
	self->cursor.y = min(max(0, self->cursor.y), SCREEN_HEIGHT - 1);

//	Easiest way to exit from here
	if(keydown(KEY_MENU)) RebootOS();

	self->collisions(map, &self->props);
}

/* Having a generic physics property struct lets me have one function to handle
collisions, instead of one for each entity/player struct */
void handleCollisions(struct Map* map, struct EntPhysicsProps* self)
{
	struct Rect tileCheckBox = {
		{
			max(0, (self->x >> 3) - 1),
			max(0, (self->y >> 3) - 1)
		},
		{
			min(MAP_WIDTH - 1, ((self->x + self->width) >> 3) + 1),
			min(MAP_HEIGHT - 1, ((self->y + self->height) >> 3) + 1)
		}
	};

	int checkLeft, checkRight, checkTop, checkBottom;
	int overlapX, overlapY;

	self->yVel += GRAVITY_ACCEL;
	if(abs(self->xVel) < 1) self->xVel = 0;
	self->x += self->xVel;
	self->y += self->yVel;

	self->touchingTileTop = false;
	for(int y = tileCheckBox.TL.y; y <= tileCheckBox.BR.y; y++)
	{
		for(int x = tileCheckBox.TL.x; x <= tileCheckBox.BR.x; x++)
		{
			if(map->tiles[y * MAP_WIDTH + x]->solid)
			{
				struct Rect entBox = {
					{
						self->x,
						self->y
					},
					{
						self->x + self->width - 1,
						self->y + self->height - 1
					}
				};

//				These can be changed when I implement non-fullheight blocks
				checkLeft = x << 3;
				checkRight = checkLeft + 7;
				checkTop = y << 3;
				checkBottom = checkTop + 7;

				overlapX = max(0, min(entBox.BR.x, checkRight + 1) - max(entBox.TL.x, checkLeft - 1));
				overlapY = max(0, min(entBox.BR.y, checkBottom + 1) - max(entBox.TL.y, checkTop - 1));

				if(overlapX && overlapY)
				{
					if(overlapX >= overlapY)
					{
						self->yVel = 0;
						if(entBox.TL.y >= checkTop)
						{
							self->y += overlapY;
						} 
						else 
						{
							self->y -= overlapY;
							self->touchingTileTop = true;
						}
					}
					else
					{
						self->xVel = 0;
						if(entBox.TL.x <= checkLeft)
						{
							self->x -= overlapX;
						} 
						else
						{
							self->x += overlapX;
						}
					}
				}
			}
		}
	}
	self->xVel *= 0.8;
	self->x = min(max(self->x, 0), 8 * MAP_WIDTH - self->width);
	self->y = min(max(self->y, 0), 8 * MAP_HEIGHT - self->height);
	if(self->y + self->height == (MAP_HEIGHT << 3) - 1)
	{
		self->yVel = 0;
		self->touchingTileTop = true;
	}
}