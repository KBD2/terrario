#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/defs/util.h>
#include <math.h>
#include <gint/std/stdlib.h>

#include "update.h"
#include "defs.h"
#include "inventory.h"
#include "entity.h"
#include "world.h"
#include "crafting.h"
#include "render.h"

// Update player and do keyboard stuff
enum UpdateReturnCodes keyboardUpdate()
{
	bool validLeft, validRight, validTop, validBottom;
	int x, y;
	key_event_t key;
	enum Tiles tile;
	bool playerDead =  player.combat.health <= 0;
	struct Chest* chest;
	struct PickData *heldPickData;
	
	player.inventory.ticksSinceInteracted++;

	key = pollevent();
	while(key.type != KEYEV_NONE)
	{
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_switch(&takeVRAMCapture);
				break;
			
			case KEY_SHIFT:
				player.inventory.ticksSinceInteracted = 0;
				if(key.type != KEYEV_DOWN || playerDead) break;
				inventoryMenu(NULL);
//				Immediately go to crafting
				if(keydown(KEY_ALPHA))
				{
					key.key = KEY_ALPHA;
					continue;
				}
				break;
			case KEY_ALPHA:
				player.inventory.ticksSinceInteracted = 0;
				if(key.type != KEYEV_DOWN || playerDead) break;
				craftingMenu();
//				Immediately go to inventory
				if(keydown(KEY_SHIFT))
				{
					key.key = KEY_SHIFT;
					continue;
				}
				break;
			
			case KEY_8:
				if(key.type == KEYEV_DOWN) 
				{
					if(player.props.touchingTileTop || (player.bonuses.doubleJump && !player.bonuses.hasDoubleJumped))
					{
						player.props.yVel = -4.5;
						player.props.dropping = true;
						if(!player.props.touchingTileTop && player.bonuses.doubleJump)
						{
							player.bonuses.hasDoubleJumped = true;
							createExplosion(&world.explosion, player.props.x + (player.props.width >> 1), player.props.y + player.props.height);
						}
					}
				}
				break;
			
			case KEY_F1: case KEY_F2: case KEY_F3: case KEY_F4: case KEY_F5:
				player.inventory.ticksSinceInteracted = 0;
				if(player.swingFrame == 0 && !playerDead) player.inventory.hotbarSlot = keycode_function(key.key) - 1;
//				Updates held tool data
				registerHeld();
				break;

			case KEY_MENU:
				if(exitMenu()) return UPDATE_EXIT;
				break;
			
			case KEY_9:
				if(key.type != KEYEV_DOWN) break;
				x = player.cursorTile.x;
				y = player.cursorTile.y;
				tile = getTile(x, y).id;
				switch(tile)
				{
					case TILE_CHEST_L: case TILE_CHEST_R:
						if(tile == TILE_CHEST_R) x--;
						while(getTile(x, y - 1).id == TILE_CHEST_L) y--;
						chest = world.chests.findChest(x, y);
						if(chest == NULL) world.chests.addChest(x, y);
						player.inventory.ticksSinceInteracted = 0;
						inventoryMenu(world.chests.findChest(x, y));
						break;

					case TILE_DOOR_C:
						openDoor(x, y);
						break;
					
					case TILE_DOOR_O_L_L: 
					case TILE_DOOR_O_L_R: 
					case TILE_DOOR_O_R_L: 
					case TILE_DOOR_O_R_R:
						closeDoor(x, y);
						break;

					default:
						break;
				}
				break;
			
			default:
				break;
		}
		key = pollevent();
	}

//	These need to run as long as the button is held

	if(!playerDead)
	{
		if(keydown(KEY_7))
		{
			if(items[player.inventory.getSelected()->id].type != TOOL_TYPE_NONE)
			{
				if(player.swingFrame == 0) player.swingFrame = 32;
				player.swingDir = player.cursor.x < 64;
				switch(player.tool.type)
				{
					case TOOL_TYPE_PICK:
						heldPickData = &player.tool.data.pickData;
						if(heldPickData->currFramesLeft == 0)
						{
							x = player.cursorTile.x;
							y = player.cursorTile.y;
							player.inventory.ticksSinceInteracted = 0;
							if(x != heldPickData->targeted.x || y != heldPickData->targeted.y)
							{
//								You can safely assume this will be called at least once.
								heldPickData->targeted.x = x;
								heldPickData->targeted.y = y;
								heldPickData->targeted.damage = 0;
								heldPickData->targeted.crackVar = rand() % 6;
							}
							heldPickData->targeted.damage += (float)heldPickData->power / tiles[getTile(x, y).id].hitpoints;
							if(heldPickData->targeted.damage >= 100)
							{
								world.removeTile(x, y);
								heldPickData->targeted.damage = 0;
							}
							else setVar(x, y);
							heldPickData->currFramesLeft = heldPickData->speed;
						}
						else heldPickData->currFramesLeft--;

					default:
						break;
				}
			}
			else
			{
				player.inventory.ticksSinceInteracted = 0;
				x = player.cursorTile.x;
				y = player.cursorTile.y;
				validLeft = x < player.props.x >> 3;
				validRight = x > (player.props.x + player.props.width) >> 3;
				validTop = y < player.props.y >> 3;
				validBottom = y > (player.props.y + player.props.height) >> 3;
				tile = items[player.inventory.getSelected()->id].tile;
				if(tile != TILE_NULL && tile != TILE_NOTHING)
				{
					if(validLeft || validRight || validTop || validBottom || tiles[tile].physics == PHYS_NON_SOLID)
					{
						world.placeTile(x, y, player.inventory.getSelected());
					}
				}
			}
		}
		else if(player.tool.type == TOOL_TYPE_PICK) player.tool.data.pickData.currFramesLeft = 0;

//		Movement
		if(keydown(KEY_4) && player.props.xVel > -1) player.props.xVel -= 0.3;
		if(keydown(KEY_6) && player.props.xVel < 1) player.props.xVel += 0.3;
		if(keydown(KEY_2)) player.props.dropping = true;
#ifdef DEBUGMODE
		if(keydown(KEY_4)) player.props.xVel = -1;
		if(keydown(KEY_6)) player.props.xVel = 1;
		if(keydown(KEY_8)) player.props.yVel = -1;
		if(keydown(KEY_2)) player.props.yVel = 1;
#endif
		if(!keydown_any(KEY_8, KEY_2, 0) || (keydown(KEY_8) && !keydown(KEY_2) && player.props.yVel >= 0)) player.props.dropping = false;

//		Cursor
		if(keydown(KEY_LEFT)) player.cursor.x--;
		if(keydown(KEY_RIGHT)) player.cursor.x++;
		if(keydown(KEY_UP)) player.cursor.y--;
		if(keydown(KEY_DOWN)) player.cursor.y++;
		player.cursor.x = min(max(0, player.cursor.x), SCREEN_WIDTH - 1);
		player.cursor.y = min(max(0, player.cursor.y), SCREEN_HEIGHT - 1);
	}

	if(keydown_all(KEY_XOT, KEY_EXP, 0)) itemMenu();

	return UPDATE_CONTINUE;
}

void playerUpdate(int frames)
{
	struct AnimationData *anim = &player.anim;
	int animFrames[][2] = {
		{0, 0},
		{1, 4},
		{5, 5},
		{6, 19}
	};
	int time;
	float regen;
	int playerXSave = player.props.x;
	int playerYSave = player.props.y;
	int damage;

	int minX = VAR_BUF_OFFSET << 3;
	int maxX = ((game.WORLD_WIDTH - VAR_BUF_OFFSET) << 3) - player.props.width;
	int minY = VAR_BUF_OFFSET << 3;
	int maxY = ((game.WORLD_HEIGHT - VAR_BUF_OFFSET) << 3) - player.props.height;

//	Handle the physics for the player
	player.physics(&player.props, frames);

//	Cap the player's position at an offset so variant buffer doesn't corrupt YRAM
	player.props.x = min(max(minX, player.props.x), maxX);
	player.props.y = min(max(minY, player.props.y), maxY);
	if(player.props.y == maxY) player.props.touchingTileTop = true;

	if(player.props.touchingTileTop && player.bonuses.doubleJump) player.bonuses.hasDoubleJumped = false;

	if(player.props.yVel < 0) player.pixelsFallen = 0;
	else
	{
		player.pixelsFallen += (player.props.y - playerYSave);
		if(player.props.touchingTileTop && player.pixelsFallen > 0)
		{
			if(player.pixelsFallen >> 3 > 25)
			{
				damage = max(1, 10 * ((player.pixelsFallen >> 3) - 25) - ceil((float)(player.combat.defense + player.bonuses.defense) / 2));
				player.combat.health = max(0, player.combat.health - damage);
				player.combat.currImmuneFrames = player.combat.immuneFrames;
				player.ticksSinceHit = 0;
			}
			player.pixelsFallen = 0;
		}
	}

//	Regen health
	if(player.combat.health < player.maxHealth)
	{
		if(player.ticksSinceHit < 1800) time = player.ticksSinceHit / 360;
		else time = min(6 + (player.ticksSinceHit - 1800) / 600, 9);
		regen = 0.5 * roundf(
			(((float)player.maxHealth / 400) * 0.85 + 0.15)
			* time
			* (player.props.xVel == 0 && player.props.yVel == 0 ? 1.25 : 0.5)
		);
		if(regen > 0 && frames % (int)(60.0/regen) == 0) player.combat.health += 1;
	}

	if(player.combat.health > 0) player.ticksSinceHit++;

//	Figure out which animation frame the player should be in right now
	if(player.props.xVel > 0)
	{
		anim->direction = 0;
	}
	else if(player.props.xVel < 0)
	{
		anim->direction = 1;
	}

	if(!player.props.touchingTileTop)
	{
		anim->animation = 2;
		anim->animationFrame = 5;
	}
	else if(player.props.xVel == 0 || (playerXSave == player.props.x && player.props.touchingTileTop))
	{
		anim->animation = 0;
		anim->animationFrame = 0;
	}
	else if(player.props.xVel != 0 && anim->animation != 3)
	{
		anim->animation = 3;
		anim->animationFrame = 6;
	}
	else 
	{
		if(frames & 1) anim->animationFrame++;
	}

	if(anim->animationFrame > animFrames[anim->animation][1]) 
	{
		anim->animationFrame = animFrames[anim->animation][0];
	}
}