#include <math.h>
#include <stdlib.h>

#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/defs/util.h>

#include "update.h"
#include "defs.h"
#include "inventory.h"
#include "entity.h"
#include "world.h"
#include "crafting.h"
#include "render.h"

void useHeld()
{
	struct PickData *heldPickData;
	const struct ItemData *itemData;
	bool validSupport;
	Item *item;
	int slot;
	int x, y;
	bool validLeft, validRight, validTop, validBottom;
	enum Tiles tile;

	if(player.useFrames > 0) return;

	itemData = &items[player.inventory.getSelected()->id];
//	Picks and swords
	if(itemData->type == TOOL_TYPE_PICK || itemData->type == TOOL_TYPE_SWORD)
	{
		if(player.swingFrame == 0) player.swingFrame = 32;
		switch(player.tool.type)
		{
			case TOOL_TYPE_PICK:
				heldPickData = &player.tool.data.pickData;
				x = player.cursorTile.x;
				y = player.cursorTile.y;
				player.inventory.ticksSinceInteracted = 0;
				if(x != heldPickData->targeted.x || y != heldPickData->targeted.y)
				{
//						You can safely assume this will be called at least once.
					heldPickData->targeted.x = x;
					heldPickData->targeted.y = y;
					heldPickData->targeted.damage = 0;
					heldPickData->targeted.crackVar = rand() % 6;
				}
				heldPickData->targeted.damage += ((float)(heldPickData->power) * prefixes[player.inventory.getSelected()->prefix].power) / tiles[getTile(x, y).id].hitpoints;
				if(heldPickData->targeted.damage >= 100)
				{
					world.removeTile(x, y);
					heldPickData->targeted.damage = 0;
				}
				else setVar(x, y);
				player.useFrames = heldPickData->speed;

			default:
				break;
		}
	}
//		Miscellaneous tools
	else if(itemData->type == TOOL_TYPE_OTHER)
	{
		switch(player.inventory.getSelected()->id)
		{
			case ITEM_MAGIC_MIRROR:
				player.props.x = player.spawn.x;
				player.props.y = player.spawn.y;
				player.pixelsFallen = 0;
				player.useFrames = 90;
				break;
			
			case ITEM_CRYST:
				if(player.maxHealth < 400)
				{
					player.maxHealth += 20;
					player.combat.health += 20;
					player.inventory.removeItem(player.inventory.getSelected());
					player.inventory.ticksSinceInteracted = 0;
					player.useFrames = 30;
				}
				break;
				
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
//		Disable placing if tile above keeps the tile below
		validSupport = y > 0 && tiles[getTile(x, y - 1).id].support != SUPPORT_KEEP;
		item = player.inventory.getSelected();
		tile = items[item->id].tile;
		if(tile != TILE_NULL && tile != TILE_NOTHING && validSupport)
		{
			if(validLeft || validRight || validTop || validBottom || tiles[tile].physics == PHYS_NON_SOLID)
			{
				if(item->id == ITEM_WATER_BUCKET)
				{
					slot = player.inventory.getFirstFreeSlot(ITEM_EMPTY_BUCKET);
					player.inventory.stackItem(&player.inventory.items[slot], &(Item){ITEM_EMPTY_BUCKET, PREFIX_NONE, 1});
				}
				world.placeTile(x, y, item);
				registerGhost();
			}
		}
		else if(item->id == ITEM_EMPTY_BUCKET && getTile(x, y).id == TILE_WATER)
		{
			slot = player.inventory.getFirstFreeSlot(ITEM_WATER_BUCKET);
			player.inventory.stackItem(&player.inventory.items[slot], &(Item){ITEM_WATER_BUCKET, PREFIX_NONE, 1});
			player.inventory.removeItem(item);
			setTile(x, y, TILE_NOTHING);
			regionChange(x, y);
		}
	}
}

// Update player and do keyboard stuff
enum UpdateReturnCodes keyboardUpdate()
{
	int x, y;
	key_event_t key;
	enum Tiles tile;
	bool playerDead =  player.combat.health <= 0;
	struct Chest* chest;
	int ret;
	NPC *npc;
	
	player.inventory.ticksSinceInteracted++;

	key = pollevent();
	while(key.type != KEYEV_NONE)
	{
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_world_switch(GINT_CALL(&takeVRAMCapture));
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
				registerGhost();
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
				registerGhost();
				break;
			
			case KEY_8:
				if(key.type == KEYEV_DOWN) 
				{
					if(player.props.touchingTileTop || (player.bonuses.doubleJump && !player.bonuses.hasDoubleJumped))
					{
						player.props.dropping = true;
						player.jumpTimer = 0;
						player.jumpReleased = false;
						if(!player.props.touchingTileTop && player.bonuses.doubleJump)
						{
							player.bonuses.hasDoubleJumped = true;
							resetExplosion(player.props.x + (player.props.width >> 1), player.props.y + player.props.height);
						}
					}
				}
				break;
			
			case KEY_F1: case KEY_F2: case KEY_F3: case KEY_F4: case KEY_F5:
				player.inventory.ticksSinceInteracted = 0;
				if(player.useFrames == 0 && !playerDead) player.inventory.hotbarSlot = keycode_function(key.key) - 1;
//				Updates held item data
				registerHeld();
				break;

			case KEY_MENU:
				ret = exitMenu();
				switch(ret)
				{
					case 0:
						break;

					case 1:
						return UPDATE_EXIT;
					
					case 2:
						return UPDATE_EXIT_NOSAVE;
				}
				break;
			
			case KEY_9:
				if(key.type != KEYEV_DOWN) break;

//				Check NPCs
				x = player.cursorWorld.x;
				y = player.cursorWorld.y;
				for(int idx = 0; idx < world.numNPCs; idx++)
				{
					npc = &world.npcs[idx];
					if(x >= npc->props.x
					&& x < npc->props.x + npc->props.width
					&& y >= npc->props.y
					&& y < npc->props.y + npc->props.height)
					{
						if(npcTalk(npc->numInteractDialogue, npc->interactDialogue, npc->menuType)) npc->menu();
						break;
					}
				}

//				Check interactable tiles
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
			
			case KEY_TAN:
				if(key.type != KEYEV_DOWN) break;
				if(getTile(player.cursorTile.x, player.cursorTile.y).id != TILE_NOTHING) break;
				if(checkHousingValid(player.cursorTile))
				{
					addMarker(player.cursorTile);
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
//		Fun debug thing, will remove sometime
//		-----
		static int which = 0;
		if(keydown(KEY_DOT))
		{
			x = player.cursorTile.x;
			y = player.cursorTile.y;
			if(which == 0)
			{
				if(getTile(x, y).id == TILE_NOTHING) which = 1;
				else which = 2;
			}
			if(getTile(x, y).id == TILE_NOTHING && which == 1) setTile(x, y, TILE_WATER);
			else if(getTile(x, y).id == TILE_WATER && which == 2) setTile(x, y, TILE_NOTHING);
		}
		else which = 0;
//		-----
		if(keydown(KEY_8) && !player.jumpReleased && player.jumpTimer < 15)
		{
			player.props.yVel = -3.5;
			player.jumpTimer++;
		}
		else player.jumpReleased = true;
		if(keydown(KEY_7))
		{
			useHeld();
		}

//		Movement
		player.props.movingSelf = keydown_any(KEY_4, KEY_6, 0);
		if(keydown(KEY_4) && player.props.xVel > -1 * player.bonuses.speedBonus) player.props.xVel -= 0.2;
		if(keydown(KEY_6) && player.props.xVel < 1 * player.bonuses.speedBonus) player.props.xVel += 0.2;
		if(keydown(KEY_2)) player.props.dropping = true;
#ifdef DEBUGMODE
		if(keydown(KEY_4)) player.props.xVel = -1;
		if(keydown(KEY_6)) player.props.xVel = 1;
		if(keydown(KEY_8)) player.props.yVel = -1;
		if(keydown(KEY_2)) player.props.yVel = 1;
#endif
		if(!keydown_any(KEY_8, KEY_2, 0) || (!keydown(KEY_2) && player.props.yVel >= 0)) player.props.dropping = false;

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

	int playerYSave = player.props.y;
	static int playerXChangeTicks = 0;

	int damage;

	int minX = VAR_BUF_OFFSET << 3;
	int maxX = ((game.WORLD_WIDTH - VAR_BUF_OFFSET) << 3) - player.props.width;
	int minY = VAR_BUF_OFFSET << 3;
	int maxY = ((game.WORLD_HEIGHT - VAR_BUF_OFFSET) << 3) - player.props.height;

	if(player.useFrames > 0) player.useFrames--;

//	Handle the physics for the player
	handlePhysics(&player.props, frames, false, WATER_FRICTION);

//	Cap the player's position at an offset so variant buffer doesn't corrupt YRAM
	player.props.x = min(max(minX, player.props.x), maxX);
	player.props.y = min(max(minY, player.props.y), maxY);
	if(player.props.y == maxY) player.props.touchingTileTop = true;

	if(player.props.touchingTileTop && player.bonuses.doubleJump) player.bonuses.hasDoubleJumped = false;

//	Fall damage
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

//	Breath/drowning
	if(checkEntitySubmerged(&player.props, 3))
	{
		if(frames % 7 == 0)
		{
			if(player.breath > 0) player.breath--;
			else
			{
				player.combat.health = max(0, player.combat.health - 2);
				player.ticksSinceHit = 0;
			}
		}
	}
	else if(player.breath < 200) player.breath++;

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
	else if(player.props.xVel == 0)
	{
		anim->animation = 0;
		anim->animationFrame = 0;
		playerXChangeTicks = 0;
	}
	else if(player.props.xVel != 0 && anim->animation != 3)
	{
		if(playerXChangeTicks < 5) playerXChangeTicks++;
		else
		{
			anim->animation = 3;
			anim->animationFrame = 6;
		}
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

void worldUpdate()
{
	int tempY;
	enum Tiles tile;
	int placeX, placeY;
	enum SupportTypes support;
	int virtX, virtY;

	for(int y = min(game.WORLD_HEIGHT - 1, (player.props.y >> 3) + 10); y > max(0, (player.props.y >> 3) - 10); y--)
	{
		for(int x = max(1, (player.props.x >> 3) - 10); x < min(game.WORLD_WIDTH - 1, (player.props.x >> 3) + 10); x++)
		{
			tile = getTile(x, y).id;
//			Drop sand tiles
			if(tiles[tile].physics == PHYS_SAND && getTile(x, y + 1).id == TILE_NOTHING)
			{
				for(tempY = y; tempY >= 0 && tiles[getTile(x, tempY).id].physics == PHYS_SAND; tempY--)
				{
					support = tiles[getTile(x, tempY - 1).id].support;
					if(support == SUPPORT_NEED) world.removeTile(x, tempY - 1);
					else if(support == SUPPORT_KEEP) break;
					setTile(x, tempY + 1, getTile(x, tempY).id);
					setVar(x, tempY + 1);
				}
				setTile(x, tempY + 1, TILE_NOTHING);
			}
//			Water tile physics
			else if(tile == TILE_WATER)
			{
				placeX = -1;
				if(tiles[getTile(x, y + 1).id].canFlood)
				{
					placeX = x;
					placeY = y + 1;
				}
				else if(rand() % 2)
				{
					if(tiles[getTile(x - 1, y + 1).id].canFlood)
					{
						placeX = x - 1;
						placeY = y + 1;
					}
					else if(tiles[getTile(x - 1, y).id].canFlood)
					{
						placeX = x - 1;
						placeY = y;
					}
				}
				else
				{
					if(tiles[getTile(x + 1, y + 1).id].canFlood)
					{
						placeX = x + 1;
						placeY = y + 1;
					}
					else if(tiles[getTile(x + 1, y).id].canFlood)
					{
						placeX = x + 1;
						placeY = y;
					}
				}
				if(placeX > 0)
				{
					setTile(x, y, TILE_NOTHING);
					if(getTile(placeX, placeY).id != TILE_NOTHING) removeTile(placeX, placeY);
					setTile(placeX, placeY, TILE_WATER);
					regionChange(x, y);
					regionChange(placeX, placeY);
					setVar(placeX, placeY);
				}
			}
//			Grass spread
			else if(tile == TILE_DIRT)
			{
				if(findState(x, y) != 0xf
				&& (getTile(x - 1, y).id == TILE_GRASS 
					|| getTile(x + 1, y).id == TILE_GRASS
					|| getTile(x, y + 1).id == TILE_GRASS
					|| getTile(x, y - 1).id == TILE_GRASS)
				&& rand() % 275 == 0) setTile(x, y, TILE_GRASS);
			}
//			Torch animation
			else if(tile == TILE_TORCH)
			{
				virtX = x - varBufferPos.x;
				virtY = y - varBufferPos.y;
				if(varBuffer[virtY][virtX] == 2) varBuffer[virtY][virtX] = 0;
				else varBuffer[virtY][virtX]++;
			}
		}
	}
}
