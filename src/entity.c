#include <gint/keyboard.h>
#include <gint/defs/util.h>

#include <gint/gray.h>
#include <gint/std/stdio.h>

#include "syscalls.h"
#include "entity.h"
#include "map.h"

void updatePlayer(struct Map* map, struct Player* self)
{
	clearevents();
	if(keydown(KEY_LEFT)) self->props.xVel = -4;
	if(keydown(KEY_RIGHT)) self->props.xVel = 4;
	if(keydown(KEY_UP) && self->props.touchingTileTop) self->props.yVel = -6;
//	Easiest way to exit from here
	if(keydown(KEY_MENU)) RebootOS();
	self->collisions(map, &self->props);
	self->props.x = min(max(self->props.x, 0), 8 * MAP_WIDTH - self->props.width);
	self->props.y = min(max(self->props.y, 0), 8 * MAP_HEIGHT - self->props.height);
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
			min(MAP_WIDTH - 1, ((self->x + self->width) >> 3) - 1),
			min(MAP_HEIGHT - 1, ((self->y + self->height) >> 3) + 1)
		}
	};

	int checkLeft, checkRight, checkTop, checkBottom;
	bool overlapLeft, overlapRight, overlapTop, overlapBottom;

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

				overlapLeft = entBox.TL.x <= checkRight && entBox.BR.x >= checkLeft;
				overlapRight = entBox.BR.x >= checkLeft && entBox.TL.x <= checkRight;
				overlapTop = entBox.TL.y <= checkBottom && entBox.BR.y >= checkTop;
				overlapBottom = entBox.BR.y >= checkTop && entBox.TL.y <= checkBottom;
				
				if((overlapLeft || overlapRight) && (overlapTop || overlapBottom))
				{
					if(overlapBottom)
					{
						self->y -= entBox.BR.y - checkTop;
						self->yVel = 0;
						self->touchingTileTop = true;
					} else if(overlapTop)
					{
						self->y += checkBottom - entBox.TL.y;
						self->yVel = 0;
					} else if(overlapLeft)
					{
						self->x += checkRight - entBox.TL.x;
						self->xVel = 0;
					} else if(overlapRight)
					{
						self->x -= entBox.BR.x - checkLeft;
						self->xVel = 0;
					}
				}
			}
		}
	}
	if(self->touchingTileTop) self->xVel *= 0.8;
}