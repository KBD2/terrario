#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/gray.h>
#include <stdbool.h>
#include <math.h>
#include <gint/timer.h>
#include <gint/gint.h>
#include <gint/std/stdlib.h>

#include "entity.h"
#include "defs.h"
#include "world.h"
#include "render.h"
#include "inventory.h"
#include "menu.h"

extern bopti_image_t
img_ent_slime;

void slimeInit(struct EntityBase *self)
{
	self->mem[2] = rand() % 2;
}

bool slimeBehaviour(struct EntityBase *self, GUNUSED int frames)
{
	int *jumpTimer = &self->mem[0];
	int *animTimer = &self->mem[1];
	int *direction = &self->mem[2];

	handlePhysics(&self->props);

	if(self->props.touchingTileTop && *jumpTimer == 0)
	{
		*jumpTimer = 240;
	}
	else if(!self->props.touchingTileTop) *jumpTimer = 0;
	else if(*jumpTimer > 0)
	{
		(*jumpTimer)--;
		if(*jumpTimer == 0)
		{
			self->props.yVel = -4.5;
			self->props.xVel = *direction == 0 ? -3 : 3;
			self->anim.animationFrame = 1;
		}
	}

	if(*animTimer == 0 && self->props.touchingTileTop)
	{
		self->anim.animationFrame = !self->anim.animationFrame;
		if(*jumpTimer > 0 && *jumpTimer <= 60) *animTimer = 8;
		else *animTimer = 32;
	}
	else if(self->props.touchingTileTop) (*animTimer)--;
	return true;
}

const struct EntityBase entityTemplates[] = {
//		ID			Memory	Props		Anim	Combat										Sprite			Behaviour			Init
	{	ENT_SLIME,	{ 0 },	{16, 12},	{ 0 },	{14, ALIGN_HOSTILE, 40, 0, 6, 0, 0.15},	&img_ent_slime,	&slimeBehaviour,	&slimeInit	}	// ENT_SLIME
};

/* Having a generic physics property struct lets me have one function to handle
collisions, instead of one for each entity/player struct */
void handlePhysics(struct EntityPhysicsProps *self)
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
	if(abs(self->xVel) < 0.1) self->xVel = 0;
	self->x += roundf(self->xVel);
	self->y += roundf(self->yVel);
	self->y++;

	self->touchingTileTop = false;
	for(int y = tileCheckBox.TL.y; y <= tileCheckBox.BR.y; y++)
	{
		for(int x = tileCheckBox.TL.x; x <= tileCheckBox.BR.x; x++)
		{
			if(tiles[getTile(x, y).idx].physics != PHYS_NON_SOLID)
			{
				if(tiles[getTile(x, y).idx].physics == PHYS_PLATFORM && (y < ((self->y + self->height) >> 3) || self->dropping)) continue;

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

//	Friction
	if(self->touchingTileTop) self->xVel *= 0.7;
	else self->xVel *= 0.95;

	if(self->x < 0 || self->x > (WORLD_WIDTH - self->width) << 3)
	{
		self->xVel = 0;
		self->x = min(max(self->x, 0), (WORLD_WIDTH - self->width) << 3);
	}
	if(self->y < 0 || self->y > (WORLD_HEIGHT - self->height) << 3)
	{
		self->yVel = 0;
		self->y = min(max(self->y, 0), (WORLD_HEIGHT - self->height) << 3);
	}
	if(self->y + self->height >= (WORLD_HEIGHT << 3) - 1)
	{
		self->yVel = 0;
		self->touchingTileTop = true;
	}
}

bool checkCollision(struct EntityPhysicsProps *first, struct EntityPhysicsProps *second)
{
	return (
		first->x + first->width > second->x
		&& first->x < second->x + second->width
		&& first->y + first->height > second->y
		&& first->y < second->y + second->height
	);
}

// Assumes entities never attack each other
void attack(Entity *entity, bool isPlayerAttacking)
{
	struct EntityPhysicsProps *attackerProps, *defenderProps;
	struct Combat *attackerCombat, *defenderCombat;

	attackerProps = isPlayerAttacking ? &player.props : &entity->props;
	attackerCombat = isPlayerAttacking ? &player.combat : &entity->combat;

	defenderProps = isPlayerAttacking ? &entity->props : &player.props;
	defenderCombat = isPlayerAttacking ? &entity->combat : &player.combat;

	defenderCombat->health -= (attackerCombat->attack - ceil((float)defenderCombat->defense / 2));

	defenderCombat->currImmuneFrames = defenderCombat->immuneFrames;
	defenderProps->yVel = -3.0 * (1.0 - defenderCombat->knockbackResist);
	defenderProps->xVel = (4.0 * sgn(defenderProps->x - attackerProps->x)) * (1.0 - defenderCombat->knockbackResist);
}

void doEntityCycle(int frames)
{
	Entity* ent;
	struct EntityPhysicsProps weaponProps = { 0 };

	if(player.swingFrame > 0)
	{
		switch(player.inventory.getSelected()->id)
		{
			case ITEM_SWORD:
				player.combat.attack = 5;
				break;
			
			default:
				player.combat.attack = 0;
		}

		weaponProps = (struct EntityPhysicsProps) {
			.x = player.props.x + (player.swingDir ? -16 : 0),
			.y = player.props.y - 16,
			.width = 16 + player.props.width,
			.height = player.props.height + 16
		};
	}

	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id != -1)
		{
			ent = &world.entities[idx];
			ent->behaviour(&world.entities[idx], frames);
			if(player.combat.health > 0 && player.combat.currImmuneFrames == 0)
			{
				if(checkCollision(&ent->props, &player.props))
				{
					attack(ent, false);
					if(player.combat.health <= 0) createExplosion(&world.explosion, player.props.x + (player.props.width >> 1), player.props.y + (player.props.height >> 1));
				}
			}
			if(ent->combat.currImmuneFrames > 0) ent->combat.currImmuneFrames--;
			else
			{
				if(checkCollision(&weaponProps, &ent->props)) attack(ent, true);
				if(ent->combat.health <= 0)
				{
					createExplosion(&world.explosion, ent->props.x + (ent->props.width >> 1), ent->props.y + (ent->props.height >> 1));
					world.removeEntity(idx);
				}
			}
		}
	}
	if(player.combat.currImmuneFrames > 0) player.combat.currImmuneFrames--;
	if(player.swingFrame > 0) player.swingFrame--;
}