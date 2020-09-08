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

/* ----- ENTITY DATA AND BEHAVIOUR DEFINITIONS ----- */

// SLIMES

const struct EntityDrops slimeDrops = {
	.num = 1,
	.dropList = (const Drop[]){
//		 Item		Min Max Low Hi
		{ITEM_GEL,	1,	2,	1,	1}
	}
};

void slimeInit(struct EntityBase *self)
{
	self->mem[2] = rand() % 2;
}

bool slimeBehaviour(struct EntityBase *self, GUNUSED int frames)
{
	int *jumpTimer = &self->mem[0];
	int *animTimer = &self->mem[1];
	int *direction = &self->mem[2];
	int *angry = &self->mem[3];

	handlePhysics(&self->props);

	if(*angry)
	{
		*direction = self->props.x < player.props.x;
	}

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

/* ---------- */

const struct EntityBase entityTemplates[] = {
//		ID			Props		Combat									Sprite			Drops			Behaviour			Init
	{	ENT_SLIME,	{16, 12},	{14, ALIGN_HOSTILE, 40, 0, 6, 0, 0.15},	&img_ent_slime,	&slimeDrops,	&slimeBehaviour,	&slimeInit	}	// ENT_SLIME
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

	int xMax = (WORLD_WIDTH << 3) - self->width;
	int yMax = (WORLD_HEIGHT << 3) - self->height;

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
						//self->xVel = 0;
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

	if(self->x < 0 || self->x > xMax)
	{
		self->xVel = 0;
		self->x = min(max(self->x, 0), xMax);
	}
	if(self->y < 0 || self->y > yMax)
	{
		self->yVel = 0;
		self->y = min(max(self->y, 0), yMax);
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
	defenderCombat->health = max(0, defenderCombat->health);

	defenderCombat->currImmuneFrames = defenderCombat->immuneFrames;
	defenderProps->yVel = -3.0 * (1.0 - defenderCombat->knockbackResist);
	defenderProps->xVel = (4.0 * sgn(defenderProps->x - attackerProps->x)) * (1.0 - defenderCombat->knockbackResist);
}

void doEntityDrop(const struct EntityDrops *drops)
{
	Item hold;
	int amount;
	const Drop *currDrop;
	int freeSlot;

	for(int drop = 0; drop < drops->num; drop++)
		{
			currDrop = &drops->dropList[drop];
			if(rand() % currDrop->ratioHigh >= currDrop->ratioLow - 1)
			{
				amount = (rand() % (currDrop->amountMax - currDrop->amountMin + 1)) + currDrop->amountMin;
				hold = (Item){currDrop->item, amount};

				while(hold.id != ITEM_NULL)
				{
					freeSlot = player.inventory.getFirstFreeSlot(currDrop->item);
					if(freeSlot > -1)
					{
						player.inventory.stackItem(&player.inventory.items[freeSlot], &hold);
					}
					else break;
				}
			}
		}
}

void doEntityCycle(int frames)
{
	Entity *ent;
	struct EntityPhysicsProps weaponProps = { 0 };

	if(player.swingFrame > 0)
	{
		switch(player.inventory.getSelected()->id)
		{
			case ITEM_COPPER_SWORD:
				player.combat.attack = 8;
				break;

			case ITEM_COPPER_PICK:
				player.combat.attack = 4;
				break;
			
			default:
				player.combat.attack = 0;
				break;
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
				if(checkCollision(&ent->props, &player.props)) attack(ent, false);
			}

			if(ent->combat.currImmuneFrames > 0) ent->combat.currImmuneFrames--;
			else
			{
				if(checkCollision(&weaponProps, &ent->props))
				{
					attack(ent, true);
					switch(ent->id)
					{
						case ENT_SLIME:
							ent->mem[3] = 1;
							break;

						default:
							break;
					}
				}
				if(ent->combat.health <= 0)
				{
					doEntityDrop(ent->drops);
					createExplosion(&world.explosion, ent->props.x + (ent->props.width >> 1), ent->props.y + (ent->props.height >> 1));
					world.removeEntity(idx);
				}
			}
		}
	}
	if(player.combat.currImmuneFrames > 0) player.combat.currImmuneFrames--;
	if(player.swingFrame > 0) player.swingFrame--;
}

void doSpawningCycle()
{
	Entity* ent;
	int spawnX, spawnY;
	int playerTileX = player.props.x >> 3;
	int playerTileY = player.props.y >> 3;
	int spawnAttempts = 0;

	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id == -1) continue;

		ent = &world.entities[idx];

		if(abs(ent->props.x - player.props.x) > 256 || abs(ent->props.y - player.props.y) > 128)
		{
			world.removeEntity(idx);
			continue;
		}

		if(abs(ent->props.x - player.props.x) > 64 || abs(ent->props.y - player.props.y) > 32)
		{
			ent->despawnCounter++;
			if(ent->despawnCounter == 750)
			{
				world.removeEntity(idx);
				continue;
			}
		}
		else if(ent->despawnCounter > 0) ent->despawnCounter = 0;
	}

	if(rand() % SPAWN_CHANCE == 0)
	{
		if(!world.checkFreeEntitySlot()) return;

		while(true)
		{
			spawnAttempts++;
//			Too low and it won't spawn enough
//			Too high and it may lag the calc
			if(spawnAttempts == 100) return;

			spawnX = playerTileX + ((rand() % 33) - 16);
			spawnX = min(max(spawnX, 0), WORLD_WIDTH - 1);

			spawnY = playerTileY + ((rand() % 25) - 12);
			spawnY = min(max(spawnY, 0), WORLD_HEIGHT - 1);

			if(tiles[getTile(spawnX, spawnY).idx].physics == PHYS_SOLID) continue;

			while(spawnY < WORLD_HEIGHT)
			{
				spawnY++;
				if(tiles[getTile(spawnX, spawnY).idx].physics == PHYS_SOLID)
				{
					if(abs(spawnX - playerTileX) < 9 && abs(spawnY - playerTileY) < 9) break;
					world.spawnEntity(ENT_SLIME, spawnX << 3, (spawnY << 3) - entityTemplates[ENT_SLIME].props.height);
				}
			}
		}
	}
}