#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/defs/util.h>
#include <math.h>

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
	int currID;

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
				inventoryMenu();
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
			
			case KEY_F1: case KEY_F2: case KEY_F3: case KEY_F4: case KEY_F5:
				player.inventory.ticksSinceInteracted = 0;
				if(player.swingFrame == 0 && !playerDead) player.inventory.hotbarSlot = keycode_function(key.key) - 1;
				currID = player.inventory.getSelected()->id;
				switch(currID)
				{
					case ITEM_COPPER_PICK:
						player.tool.type = TOOL_TYPE_PICK;
						for(int i = 0; i < NUM_PICKS; i++)
						{
							if(pickMap[i][0] == currID) player.tool.data.pickData = pickData[pickMap[i][1]];
						}
						break;
					
					case ITEM_COPPER_SWORD:
						player.tool.type = TOOL_TYPE_SWORD;
						for(int i = 0; i < NUM_SWORDS; i++)
						{
							if(swordMap[i][0] == currID) player.tool.data.swordData = swordData[swordMap[i][1]];
						}
						break;
					
					default:
						player.tool.type = TOOL_TYPE_NONE;
						break;
				}
				break;

			case KEY_MENU:
				if(exitMenu()) return UPDATE_EXIT;
				break;
			
			default:
				break;
		}
		key = pollevent();
	}

//	These need to run as long as the button is held

	if(!playerDead)
	{
//		Remove tile
		if(keydown(KEY_7) && items[player.inventory.getSelected()->id].canSwing)
		{
			if(player.swingFrame == 0) player.swingFrame = 32;
			player.swingDir = player.cursorTile.x < player.props.x >> 3;
			switch(player.tool.type)
			{
				case TOOL_TYPE_PICK:
					if(player.tool.data.pickData.currFramesLeft == 0)
					{
						x = player.cursorTile.x;
						y = player.cursorTile.y;
						player.inventory.ticksSinceInteracted = 0;
						world.removeTile(x, y);
						player.tool.data.pickData.currFramesLeft = player.tool.data.pickData.speed;
					}
					else player.tool.data.pickData.currFramesLeft--;

				default:
					break;
			}
		}
		else if(player.tool.type == TOOL_TYPE_PICK) player.tool.data.pickData.currFramesLeft = 0;

//		Place tile
		if(keydown(KEY_9))
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
				if(validLeft || validRight || validTop || validBottom || tiles[tile].physics != PHYS_SOLID)
				{
					world.placeTile(x, y, player.inventory.getSelected());
				}
			}
		}

//		Movement
		if(keydown(KEY_4) && player.props.xVel > -1) player.props.xVel -= 0.3;
		if(keydown(KEY_6) && player.props.xVel < 1) player.props.xVel += 0.3;
		if(keydown(KEY_8) && player.props.touchingTileTop) 
		{
			player.props.yVel = -4.5;
			player.props.dropping = true;
		}
		if(keydown(KEY_2)) player.props.dropping = true;
		if(!keydown_any(KEY_8, KEY_2, 0) || (keydown(KEY_8) && !keydown(KEY_2) && player.props.yVel <= 0)) player.props.dropping = false;

//		Cursor
		if(keydown(KEY_LEFT)) player.cursor.x--;
		if(keydown(KEY_RIGHT)) player.cursor.x++;
		if(keydown(KEY_UP)) player.cursor.y--;
		if(keydown(KEY_DOWN)) player.cursor.y++;
		player.cursor.x = min(max(0, player.cursor.x), SCREEN_WIDTH - 1);
		player.cursor.y = min(max(0, player.cursor.y), SCREEN_HEIGHT - 1);
	}

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

//	Handle the physics for the player
	player.physics(&player.props);

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

	player.ticksSinceHit++;

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
	else if(player.props.xVel != 0 && anim->animation != 3)
	{
		anim->animation = 3;
		anim->animationFrame = 6;
	}
	else if(player.props.xVel == 0)
	{
		anim->animation = 0;
		anim->animationFrame = 0;
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