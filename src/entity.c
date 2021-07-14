#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include <gint/timer.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gint/gray.h>

#include "entity.h"
#include "defs.h"
#include "world.h"
#include "render.h"
#include "inventory.h"
#include "menu.h"

extern bopti_image_t
img_ents_slime,
img_ents_zombie,
img_ents_vulture,
img_ents_demoneye;

bool checkEntitySubmerged(struct EntityPhysicsProps *props, int offsetY)
{
//	Do it lazily and only check a single tile
	int checkPixelY = props->y + offsetY;
	int checkTileY = checkPixelY >> 3;

	return getTile(props->x >> 3, checkTileY).id == TILE_WATER;
}

/* ----- ENTITY DATA AND BEHAVIOUR DEFINITIONS ----- */

// SLIMES

const struct EntityDrops slimeDrops = {
	.num = 2,
	.dropList = (const Drop[]){
//		 Item				Min Max Low Hi
		{ITEM_GEL,			1,	2,	1,	1},
		{ITEM_COIN_COPPER,	3,	3,	1,	1},
	}
};

void slimeInit(struct EntityBase *self)
{
	self->mem[2] = rand() % 2;
}

bool slimeBehaviour(struct EntityBase *self, int frames)
{
	int *jumpTimer = &self->mem[0];
	int *animTimer = &self->mem[1];
	int *direction = &self->mem[2];
	int *angry = &self->mem[3];
	int *xSave = &self->mem[4];

	handlePhysics(&self->props, frames, false, WATER_FLOAT);

	if(*angry)
	{
		*direction = self->props.x < player.props.x;
	}

	if(self->props.touchingTileTop && *jumpTimer == 0)
	{
		*jumpTimer = 240;
		if(!*angry && self->props.x == *xSave) *direction ^= 1;
	}
	else if(!self->props.touchingTileTop) *jumpTimer = 0;
	else if(*jumpTimer > 0)
	{
		(*jumpTimer)--;
		if(*jumpTimer == 0)
		{
			self->props.yVel = -4.5;
			self->props.xVel = *direction ? 3 : -3;
			self->anim.animationFrame = 1;
			*xSave = self->props.x;
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

// ZOMBIES

const struct EntityDrops zombieDrops = {
	.num = 3,
	.dropList = (const Drop[]){
//		 Item				Min Max Low Hi
		{ITEM_SHACKLE,		1,	1,	1,	50	},
		{ITEM_ZOMBIE_ARM,	1,	1,	1,	100	}, // Was originally 1/250, increased it
		{ITEM_COIN_COPPER, 	60,	60,	1,	1	},
	}
};

void zombieInit(struct EntityBase *self)
{
	self->props.movingSelf = true;
}

bool zombieBehaviour(struct EntityBase *self, int frames)
{
	int *animFrame = &self->mem[0];
	int checkX, checkY;

	handlePhysics(&self->props, frames, false, WATER_FRICTION);

	self->anim.direction = player.props.x < self->props.x;
	(*animFrame)++;
	*animFrame %= 60;
	if(self->props.touchingTileTop) self->anim.animationFrame = *animFrame >= 30;
	else self->anim.animationFrame = 2;

	if(self->anim.direction && self->props.xVel > -0.35) self->props.xVel -= 0.1;
	else if(!self->anim.direction && self->props.xVel < 0.35) self->props.xVel += 0.1;

	self->props.dropping = self->props.yVel < 0;
	if(abs(self->props.x - player.props.x) < 8 && player.props.y > self->props.y) self->props.dropping = true;
	
	if(self->props.touchingTileTop)
	{
		checkY = (self->props.y >> 3);
		checkX = (self->props.x >> 3);
		checkX += self->anim.direction ? 0 : 2;
		if(tiles[getTile(checkX, checkY + 3).id].physics == PHYS_NON_SOLID && tiles[getTile(checkX, checkY + 4).id].physics == PHYS_NON_SOLID) self->props.yVel = -4.5;
		else
		{
			checkX += self->anim.direction ? -1 : 1;
			for(int dY = 0; dY < 2; dY++)
			{
				if(tiles[getTile(checkX, checkY + dY).id].physics != PHYS_NON_SOLID && self->props.touchingTileTop)
				{
					self->props.yVel = -4.5;
				}
			}
		}
	}

	return true;
}

// VULTURES

const struct EntityDrops vultureDrops = {
	.num = 1,
	.dropList = (const Drop[]){
//		 Item				Min Max Low Hi
		{ITEM_COIN_COPPER, 	60,	60,	1,	1	},
	}
};

void vultureInit(struct EntityBase *self)
{
	self->anim.direction = rand() % 2;
	self->props.dropping = true;
	self->props.movingSelf = true;
}

bool vultureBehaviour(struct EntityBase *self, int frames)
{
	int offsetX, offsetY;
	int distanceSquared;
	int *mode = &self->mem[0];
	int *direction = &self->mem[1];
	int *dropTimer = &self->mem[2];
	int *ySave = &self->mem[3];
	int *blockTest = &self->mem[4];

	handlePhysics(&self->props, frames, true, WATER_FRICTION);
	if(!*mode)
	{
//		Sit around till the player comes too close
		offsetX = self->props.x + (self->props.width >> 1) - (player.props.x + (player.props.width >> 1));
		offsetY = self->props.y + (self->props.height >> 1) - (player.props.y + (player.props.height >> 1));
		distanceSquared = offsetX * offsetX + offsetY * offsetY;
		if(distanceSquared < 1600) *mode = 1;
	}
	else
	{
//		Try flying back if it's blocked
		if(!*direction && self->props.y == *ySave)
		{
			if(*blockTest == 5)
			{
				*dropTimer = 0;
				*direction = 1;
			}
			else (*blockTest)++;
		}
		else *blockTest = 0;
		*ySave = self->props.y;

		self->props.yVel = 0.6 * (*direction ? -1 : 1);
		(*dropTimer)++;
		if(*dropTimer >= 120)
		{
			if((!*direction && player.props.y - self->props.y < 0) || (*direction && player.props.y - self->props.y > 48))
			{
				*dropTimer = 0;
				*direction ^= 1;
			}
		}

		self->anim.direction = self->props.x + (self->props.width >> 1) > player.props.x + (player.props.width >> 1);

		self->props.xVel += 0.1 * (self->anim.direction ? -1 : 1);
		self->props.xVel = min(max(-0.6, self->props.xVel), 0.6);

		if(frames % 4 == 0)
		{
			self->anim.animationFrame++;
			if(self->anim.animationFrame == 6) self->anim.animationFrame = 1;
		}
	}
	return true;
}

// DEMONIC EYES

const struct EntityDrops demoneyeDrops = {
	.num = 2,
	.dropList = (const Drop[]){
//		 Item				Min Max Low Hi
		{ITEM_LENS,			1,	1,	1,	3	},
		{ITEM_COIN_COPPER, 	75,	75,	1,	1	},
	}
};

void demoneyeInit(struct EntityBase *self)
{
	self->anim.direction = 0;
	self->anim.animationFrame = 0;
	self->props.xVel = self->props.yVel = 0;
	self->props.dropping = true;
	self->props.movingSelf = true;
	self->mem[0] = self->mem[1] = 0;
}

#define DEMONEYE_MIN_DST 16
#define DEMONEYE_MIN_VEL 0.1
#define DEMONEYE_S_TICKS 30

bool demoneyeBehaviour(struct EntityBase *self, int frames)
{
	int *mode = &self->mem[0];
	int *tick = &self->mem[1];

	handlePhysics(&self->props, frames, true, WATER_FRICTION);

	if(!*mode)
	{
		// Go to player

		float xOff = player.props.x - self->props.x;
		float yOff = player.props.y - self->props.y;

		float dist = sqrtf(xOff * xOff + yOff * yOff);

		if(dist != 0)
		{
			xOff /= dist;
			yOff /= dist;
		}

		self->props.xVel = xOff * 0.75;
		self->props.yVel = yOff * 0.75;

		// self->props.xVel = min(max(-0.6, self->props.xVel), 0.6);
		// self->props.yVel = min(max(-0.6, self->props.yVel), 0.6);

		if(dist < DEMONEYE_MIN_DST)
		{
			*tick = 0;
			*mode = 1;
		}
	}
	else
	{
		// Got too close, run away upwards, keeping the X velocity

		self->props.yVel -= 0.01;
		self->props.yVel = min(max(-0.6, self->props.yVel), 0.6);

		if(*tick > DEMONEYE_S_TICKS)
		{
			*mode = 0;
		}

		(*tick)++;
	}

	int xDir = 1;
	if(self->props.xVel >= DEMONEYE_MIN_VEL) xDir = 2;
	else if(self->props.xVel <= -DEMONEYE_MIN_VEL) xDir = 0;

	int yDir = 1;
	if(self->props.yVel >= DEMONEYE_MIN_VEL) yDir = 2;
	else if(self->props.yVel <= -DEMONEYE_MIN_VEL) yDir = 0;

	const int dirTable[] = {
		5, 6, 7,
		4, 0, 0,
		3, 2, 1
	};

	self->anim.direction = dirTable[xDir + 3 * yDir];

	return true;
}

/* ---------- */

const struct EntityBase entityTemplates[] = {
//		ID				Props		Combat									Sprite				Drops			Off	Behaviour			Init
	{	ENT_SLIME,		{16, 12},	{14, ALIGN_HOSTILE, 40, 6, 0, 0.15},	&img_ents_slime,	&slimeDrops,	0,	&slimeBehaviour,	&slimeInit		},	// ENT_SLIME
	{	ENT_ZOMBIE,		{17, 23},	{45, ALIGN_HOSTILE, 40, 14, 6, 0.5}, 	&img_ents_zombie,	&zombieDrops,	0,	&zombieBehaviour,	NULL			},	// ENT_ZOMBIE
	{	ENT_VULTURE,	{18, 25},	{15, ALIGN_HOSTILE,	40, 15, 4, 0.25},	&img_ents_vulture,	&vultureDrops,	8,	&vultureBehaviour,	&vultureInit	},	// ENT_VULTURE
	{	ENT_DEMONEYE,	{18, 18},	{60, ALIGN_HOSTILE,	40, 18, 2, 0.2},	&img_ents_demoneye,	&demoneyeDrops,	0,	&demoneyeBehaviour,	&demoneyeInit	},	// ENT_DEMONEYE
};

/* Having a generic physics property struct lets me have one function to handle
collisions, instead of one for each entity/player struct */
void handlePhysics(struct EntityPhysicsProps *self, int frames, bool onlyCollisions, enum WaterPhysics water)
{
	struct Rect tileCheckBox = {
		{
			max(0, (self->x >> 3) - 1),
			max(0, (self->y >> 3) - 1)
		},
		{
			min(game.WORLD_WIDTH - 1, ((self->x + self->width) >> 3) + 1),
			min(game.WORLD_HEIGHT - 1, ((self->y + self->height) >> 3) + 1)
		}
	};

	int checkLeft, checkRight, checkTop, checkBottom;
	int overlapX, overlapY;

	int xMax = (game.WORLD_WIDTH << 3) - self->width;
	int yMax = (game.WORLD_HEIGHT << 3) - self->height;

	double integer;
	double fractional;

	int stepUpX;
	bool canStepUp;
	enum PhysicsTypes physics;

//		Friction
#ifndef DEBUGMODE
	if(!onlyCollisions)
	{
		self->y++;
		self->yVel = min(max(-4, self->yVel + GRAVITY_ACCEL), 4);

		if(checkEntitySubmerged(self, self->height - 2))
		{
			switch(water)
			{
				case WATER_FRICTION:
					self->xVel = sgn(self->xVel) * min(0.5, fabsf(self->xVel));
					self->yVel *= 0.85;
					break;
				case WATER_FLOAT:
					self->yVel -= 0.5;
					if(self->xVel == 0) self->xVel = (rand() % 2) ? 3 : -3;
					break;
			}
		}
		
		if(!self->movingSelf)
		{
			if(self->touchingTileTop) self->xVel *= 0.7;
			else if(water != WATER_FLOAT) self->xVel *= 0.95;
		}
	}
#else
	self->xVel *= 0.7;
	self->yVel *= 0.7;
#endif

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
	if(self->y + self->height >= (game.WORLD_HEIGHT << 3) - 1)
	{
		self->yVel = 0;
		self->touchingTileTop = true;
	}

	if(fabsf(self->xVel) < 0.01) self->xVel = 0;

//	Interpolate X velocity
	fractional = modf(self->xVel, &integer);
	if(fabsf(self->xVel) < 1 && frames % (int)roundf(1.0 / self->xVel) == 0) self->x += sgn(self->xVel);
	else if(fabsf(self->xVel) > 1 && frames % (int)roundf(1.0 / fractional) == 0) self->x += integer + sgn(self->xVel);
	else self->x += integer;

//	Interpolate Y velocity
	fractional = modf(self->yVel, &integer);
	if(fabsf(self->yVel) < 1 && frames % (int)roundf(1.0 / self->yVel) == 0) self->y += sgn(self->yVel);
	else if(fabsf(self->yVel) > 1 && frames % (int)roundf(1.0 / fractional) == 0) self->y += integer + sgn(self->yVel);
	else self->y += integer;

#ifndef DEBUGMODE
	self->touchingTileTop = false;
	for(int y = tileCheckBox.BR.y; y >= tileCheckBox.TL.y; y--)
	{
		for(int x = tileCheckBox.BR.x; x >= tileCheckBox.TL.x; x--)
		{
			if(tiles[getTile(x, y).id].physics != PHYS_NON_SOLID)
			{
				if(tiles[getTile(x, y).id].physics == PHYS_PLATFORM && (y < ((self->y + self->height) >> 3) || self->dropping)) continue;

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
//						Check if the entity can 'step up' a single block
						if(self->touchingTileTop)
						{
							stepUpX = self->xVel <= 0 ? (self->x - 1) >> 3 : (self->x + self->width) >> 3;
							canStepUp = true;
							for(int stepUpY = (self->y - 8) >> 3; stepUpY <= (self->y + self->height - 9) >> 3; stepUpY++)
							{
								physics = tiles[getTile(stepUpX, stepUpY).id].physics;
								if(physics == PHYS_SOLID || physics == PHYS_SAND)
								{
									canStepUp = false;
									break;
								}
							}
							if(canStepUp)
							{
								self->y -= 8;
								continue;
							}
						}

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
#endif
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
	float defense = isPlayerAttacking ? entity->combat.defense : player.combat.defense + player.bonuses.defense;

	attackerProps = isPlayerAttacking ? &player.props : &entity->props;
	attackerCombat = isPlayerAttacking ? &player.combat : &entity->combat;

	defenderProps = isPlayerAttacking ? &entity->props : &player.props;
	defenderCombat = isPlayerAttacking ? &entity->combat : &player.combat;

	defenderCombat->health -= (attackerCombat->attack - ceil(defense / 2));
	defenderCombat->health = max(0, defenderCombat->health);

	defenderCombat->currImmuneFrames = defenderCombat->immuneFrames;
	defenderProps->yVel = -3.0 * (1.0 - defenderCombat->knockbackResist);
	defenderProps->xVel = (4.0 * sgn(defenderProps->x - attackerProps->x)) * (1.0 - defenderCombat->knockbackResist);

	if(!isPlayerAttacking) player.ticksSinceHit = 0;
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
			if(rand() % currDrop->ratioHigh <= currDrop->ratioLow - 1)
			{
				amount = (rand() % (currDrop->amountMax - currDrop->amountMin + 1)) + currDrop->amountMin;
				hold = (Item){currDrop->item, rand() % PREFIX_COUNT, amount};

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
		weaponProps = (struct EntityPhysicsProps) {
			.x = player.props.x + (player.anim.direction ? -16 : 0),
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
					player.inventory.ticksSinceInteracted = 0;
					doEntityDrop(ent->drops);
					resetExplosion(ent->props.x + (ent->props.width >> 1), ent->props.y + (ent->props.height >> 1));
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
	#ifdef DEBUGMODE
	return;
	#endif

	Entity* ent;
	int spawnX, spawnY;
	int playerTileX = player.props.x >> 3;
	int playerTileY = player.props.y >> 3;
	int spawnAttempts = 0;
	enum Entities chosen;
	HouseMarker *marker;
	int dX, dY;

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

			spawnX = playerTileX + (((rand() % 49) - 24));
//			This prohibits entities from spawning below or above the player, should be properly fixed
			if(spawnX <= playerTileX) spawnX = min(max(spawnX, 0), playerTileX - 9);
			else spawnX = min(max(spawnX, playerTileX + 9), game.WORLD_WIDTH - 1);

			spawnY = playerTileY + (((rand() % 25) - 12));
			spawnY = min(max(spawnY, 0), game.WORLD_HEIGHT - 1);

			if(tiles[getTile(spawnX, spawnY).id].physics != PHYS_NON_SOLID) continue;

			while(spawnY < game.WORLD_HEIGHT)
			{
				spawnY++;
				if(tiles[getTile(spawnX, spawnY).id].physics != PHYS_NON_SOLID)
				{
					if(!isDay()) chosen = (rand() % 2) ? ENT_ZOMBIE : ENT_DEMONEYE;
					else if(getTile(spawnX, spawnY).id == TILE_SAND && rand() % 4 > 0) chosen = ENT_VULTURE;
					else chosen = ENT_SLIME;
					
					for(int idx = 0; idx < world.numMarkers; idx++)
					{
						marker = &world.markers[idx];
						if(marker->occupant == -1) continue;
						dX = spawnX - marker->position.x;
						dY = spawnY - marker->position.y;
						if(dX * dX + dY * dY < 576) return;
					}

					world.spawnEntity(chosen, spawnX << 3, (spawnY << 3) - entityTemplates[chosen].props.height);
					return;
				}
			}
		}
	}
}
