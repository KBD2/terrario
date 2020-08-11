#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/gray.h>
#include <stdbool.h>
#include <math.h>
#include <gint/timer.h>
#include <gint/gint.h>

#include "entity.h"
#include "defs.h"
#include "world.h"
#include "render.h"
#include "inventory.h"
#include "menu.h" 

/* Having a generic physics property struct lets me have one function to handle
collisions, instead of one for each entity/player struct */
void handlePhysics(struct EntPhysicsProps* self)
{
	struct Rect tileCheckBox = {
		{
			max(0, (self->x >> 3) - 1),
			max(0, (self->y >> 3) - 1)
		},
		{
			min(WORLD_WIDTH - 1, ((self->x + self->width) >> 3) + 1),
			min(WORLD_HEIGHT - 1, ((self->y + self->height) >> 3) + 1)
		}
	};

	int checkLeft, checkRight, checkTop, checkBottom;
	int overlapX, overlapY;

	self->yVel = min(10, self->yVel + GRAVITY_ACCEL);
	if((self->xVel < 0 ? -self->xVel : self->xVel) < 0.5) self->xVel = 0;
	self->x += roundf(self->xVel);
	self->y += roundf(self->yVel);
	self->y++;

	self->touchingTileTop = false;
	for(int y = tileCheckBox.TL.y; y <= tileCheckBox.BR.y; y++)
	{
		for(int x = tileCheckBox.TL.x; x <= tileCheckBox.BR.x; x++)
		{
			if(tiles[world.tiles[y * WORLD_WIDTH + x].idx].physics != PHYS_NON_SOLID)
			{
				if(tiles[world.tiles[y * WORLD_WIDTH + x].idx].physics == PHYS_PLATFORM && (y < ((player.props.y + player.props.height) >> 3) || player.props.dropping)) continue;

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
	self->x = min(max(self->x, 0), 8 * WORLD_WIDTH - self->width);
	self->y = min(max(self->y, 0), 8 * WORLD_HEIGHT - self->height);
	if(self->y + self->height >= (WORLD_HEIGHT << 3) - 1)
	{
		self->yVel = 0;
		self->touchingTileTop = true;
	}
}