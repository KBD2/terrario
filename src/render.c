#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <gint/gray.h>
#include <gint/defs/util.h>
#include <gint/bfile.h>
#include <gint/timer.h>

#include "render.h"
#include "defs.h"
#include "world.h"
#include "entity.h"
#include "inventory.h"
#include "menu.h"

Coords varBufferPos = {0, 0};

/*
For right-facing player
This compensates for the changing arm/handle position
*/
const int swingHandleDeltaPositions[4][2] = {
//		 dX 	dY
	{	-11,	-14	},
	{	6,		-13	},
	{	8,		4	},
	{	6,		12	}
};

void renderItem(int x, int y, Item *item)
{
	if (item->id < 0 || item->id >= ITEMS_COUNT) return;

	extern bopti_image_t img_items;
	int subrectX = (item->id & 0xF) * 9 + 1;
	int subrectY = (item->id >> 4) * 9 + 1;

	if(items[item->id].maxStack > 1) {
		dprint(x + 1, y + 9, C_BLACK, "%i", item->amount);
		dsubimage(x + 3, y, &img_items, subrectX, subrectY, 8, 8, DIMAGE_NONE);
	}
	else dsubimage(x + 3, y + 3, &img_items, subrectX, subrectY, 8, 8, DIMAGE_NONE);
}

void render(bool renderHUD)
{
	unsigned int camMinX = (VAR_BUF_OFFSET << 3) + (SCREEN_WIDTH >> 1);
	unsigned int camMaxX = ((game.WORLD_WIDTH - VAR_BUF_OFFSET) << 3) - (SCREEN_WIDTH >> 1);
	unsigned int camMinY = (VAR_BUF_OFFSET << 3) + (SCREEN_HEIGHT >> 1);
	unsigned int camMaxY = ((game.WORLD_HEIGHT - VAR_BUF_OFFSET) << 3) - (SCREEN_HEIGHT >> 1);
	extern bopti_image_t img_player, img_ui_cursor, img_ui_slots, img_ui_slot_highlight,
	img_leaves, img_ui_deathtext, img_bg_underground, img_sunmoon, img_bg_night,
	img_tiles_cracks, img_tiles_ghost, img_ui_bubble, img_ui_banners;
	bopti_image_t *swingSprite;
	int camX = min(max(player.props.x + (player.props.width >> 1), camMinX), camMaxX);
	int camY = min(max(player.props.y + (player.props.height >> 1), camMinY), camMaxY);

//	Translating cam bounds to tile bounds is painful
	unsigned int tileLeftX = max(0, ((camX - (SCREEN_WIDTH >> 1)) >> 3));
	unsigned int tileRightX = min(game.WORLD_WIDTH - 1, tileLeftX + (SCREEN_WIDTH >> 3));
	unsigned int tileTopY = max(0, ((camY - (SCREEN_HEIGHT >> 1)) >> 3));
	unsigned int tileBottomY = min(game.WORLD_HEIGHT - 1, tileTopY + (SCREEN_HEIGHT >> 3));

	Tile tile;
	const TileData *currTile;
	unsigned int currTileX, currTileY;
	int camOffsetX = (camX - (SCREEN_WIDTH >> 1));
	int camOffsetY = (camY - (SCREEN_HEIGHT >> 1));
	bool marginLeft, marginRight, marginTop, marginBottom;
	int flags;
	int state;

	HouseMarker *marker;

	int subrectX, subrectY;
	int entX, entY;
	int entSubrectX, entSubrectY;
	Entity *ent;
	NPC *npc;

	int x, y;

	int hotbarY;
	Item item;

	int orbX, orbY;
	float dayPolarAngle;

	int hour, minute;

	char var;

	struct PickData *heldPickData;

	Particle particle;

	player.cursorWorld.x = camX + player.cursor.x - (SCREEN_WIDTH >> 1);
	player.cursorWorld.y = camY + player.cursor.y - (SCREEN_HEIGHT >> 1);

	player.cursorTile.x = player.cursorWorld.x >> 3;
	player.cursorTile.y = player.cursorWorld.y >> 3;

	updateVarBuffer(tileLeftX, tileTopY);

	dclear(C_WHITE);

	if(player.props.y > (game.WORLD_HEIGHT / 2.8) * 8) dimage(0, 0, &img_bg_underground);
	else
	{
		if(!isDay()) dimage(0, 0, &img_bg_night);

		dayPolarAngle = (((float)PI * 2.0) / (float)DAY_TICKS) * (float)world.timeTicks;

//		Sun
		orbX = 56 * cos(dayPolarAngle + PI / 2.0) + 56;
		orbY = 64 * sin(dayPolarAngle + PI / 2.0) + 64;
		dsubimage(orbX, orbY, &img_sunmoon, 0, 0, 16, 16, DIMAGE_NONE);

//		Moon
		orbX = 56 * cos(dayPolarAngle - PI / 2.0) + 56;
		orbY = 64 * sin(dayPolarAngle - PI / 2.0) + 64;
		dsubimage(orbX, orbY, &img_sunmoon, 16, 0, 16, 16, DIMAGE_NONE);
	}

//	Do an initial pass to render treetops
	for(unsigned int y = tileTopY; y <= min(tileBottomY + 4, game.WORLD_HEIGHT - 1); y++)
	{
		for(unsigned int x = max(0, tileLeftX - 2); x <= min(tileRightX + 2, game.WORLD_WIDTH - 1); x++)
		{
			if(getTile(x, y).id == TILE_LEAVES)
			{
				currTileX = (x << 3) - camOffsetX;
				currTileY = (y << 3) - camOffsetY;
				var = varBuffer[y - varBufferPos.y][x - varBufferPos.x];
				dsubimage(currTileX - 16, currTileY - 32, &img_leaves, 41 * var + 1, 0, 40, 40, DIMAGE_NONE);
				continue;
			}
		}
	}

//	Render house markers
	for(int i = 0; i < world.numMarkers; i++)
	{
		marker = &world.markers[i];
		x = (marker->position.x << 3) - camOffsetX;
		y = (marker->position.y << 3) - camOffsetY;
		dsubimage(x, y, &img_ui_banners, 0, 0, 16, 20, DIMAGE_NONE);
//		14px x 14px max
		if(marker->occupant != -1) dimage(x + 1, y + 4, world.npcs[marker->occupant].head);
	}

//	Render tiles

	for(unsigned int y = tileTopY; y <= tileBottomY; y++)
	{
		for(unsigned int x = tileLeftX; x <= tileRightX; x++)
		{
			tile = getTile(x, y);
			currTile = &tiles[tile.id];
			currTileX = (x << 3) - camOffsetX;
			currTileY = (y << 3) - camOffsetY;
			if(currTile->render)
			{
				/* Disable clipping unless it's a block on the edges of the screen.
				This reduces rendering time a bit (edges still need clipping or
				we might crash trying to write outside the VRAM). */
				marginLeft = x - tileLeftX <= 1;
				marginRight = tileRightX - x <= 1;
				marginTop = y - tileTopY <= 1;
				marginBottom = tileBottomY - y <= 1;
				if(marginLeft | marginRight | marginTop | marginBottom)
				{
					flags = DIMAGE_NONE;
				}
				else
				{
					flags = DIMAGE_NOCLIP;
				}
				if(currTile->spriteType != TYPE_TILE)
				{
					subrectX = 0;
					subrectY = 0;
					if(currTile->spriteType == TYPE_SHEET || currTile->spriteType == TYPE_SHEET_VAR)
					{
						state = findState(x, y);
//						Spritesheet layout allows for very fast calculation of the position of the sprite
						subrectX = ((state & 3) << 3) + (state & 3) + 1;
						subrectY = ((state >> 2) << 3) + (state >> 2) + 1;
					}
					var = varBuffer[y - varBufferPos.y][x - varBufferPos.x];
					if(currTile->spriteType == TYPE_TILE_VAR) subrectX = 9 * var + 1;
					if(currTile->spriteType == TYPE_SHEET_VAR) subrectX += 37 * var;
					dsubimage(currTileX, currTileY, currTile->sprite, subrectX, subrectY, 8, 8, flags);
				}
				else
				{
					dsubimage(currTileX, currTileY, currTile->sprite, 0, 0, 8, 8, flags);
				}
			}
		}
	}

//	Render ghost object if player has an object selected
	for(int dY = 0; dY < player.ghost.height; dY++)
	{
		for(int dX = 0; dX < player.ghost.width; dX++)
		{
			tile = getTile(player.cursorTile.x + dX, player.cursorTile.y + dY);
			if(tile.id != TILE_NOTHING && tile.id != TILE_PLANT) continue;
			state = 0;
			if(player.ghost.width > 1)
			{
				if(dX < player.ghost.width - 1) state |= 0b0100;
				if(dX > 0) state |= 0b0001;
			}
			if(player.ghost.height > 1)
			{
				if(dY < player.ghost.height - 1) state |= 0b1000;
				if(dY > 0) state |= 0b0010;
			}
			subrectX = ((state & 3) << 3) + (state & 3) + 1;
			subrectY = ((state >> 2) << 3) + (state >> 2) + 1;
			dsubimage(((player.cursorTile.x + dX) << 3) - camOffsetX, ((player.cursorTile.y + dY) << 3) - camOffsetY, &img_tiles_ghost, subrectX, subrectY, 8, 8, DIMAGE_NONE);
		}
	}

//	Render cracks on the tile the player's mining
	if(player.tool.type == TOOL_TYPE_PICK && player.tool.data.pickData.targeted.damage > 0)
	{
		heldPickData = &player.tool.data.pickData;
		subrectX = heldPickData->targeted.crackVar * 9 + 1;
		subrectY = (int)(4.0 / 100.0 * heldPickData->targeted.damage) * 9 + 1;
		dsubimage(
			(heldPickData->targeted.x << 3) - camOffsetX, 
			(heldPickData->targeted.y << 3) - camOffsetY, 
			&img_tiles_cracks, 
			subrectX, subrectY, 
			8, 8, 
			DIMAGE_NONE
			);
	}

//	Render entities
	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id != -1)
		{
			ent = &world.entities[idx];

			entX = ent->props.x - (camX - (SCREEN_WIDTH >> 1));
			entY = ent->props.y - (camY - (SCREEN_HEIGHT >> 1));
			entSubrectX = ent->anim.direction ? ent->props.width + 2 * ent->spriteOffset : 0;
			entSubrectY = ent->anim.animationFrame * (ent->props.height + 1) + 1;
			dsubimage(entX - ent->spriteOffset, entY, ent->sprite, entSubrectX, entSubrectY, ent->props.width + 2 * ent->spriteOffset, ent->props.height, DIMAGE_NONE);
		}
	}

	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		npc = &world.npcs[idx];
		
		entX = npc->props.x - (camX - (SCREEN_WIDTH >> 1));
		entY = npc->props.y - (camY - (SCREEN_HEIGHT >> 1));
		entSubrectX = npc->anim.direction ? npc->props.width : 0;
//		NPCs and player have a sticky out bit at the bottom that isn't included in their height,
//		so add 2 instead of 1 when finding the subrectangle Y
		entSubrectY = npc->anim.animationFrame * (npc->props.height + 2) + 1;
		dsubimage(entX, entY, npc->sprite, entSubrectX, entSubrectY, npc->props.width, npc->props.height + 1, DIMAGE_NONE);
	}

//	Only render player if the player isn't flashing or dead
	if(!(player.combat.currImmuneFrames & 2) && player.combat.health > 0)
	{
		entX = player.props.x - (camX - (SCREEN_WIDTH >> 1)) - 2;
		entY = player.props.y - (camY - (SCREEN_HEIGHT >> 1));
		entSubrectY = player.anim.animationFrame * (player.props.height + 2) + 1;
		if(player.swingFrame == 0)
		{
			entSubrectX = !player.anim.direction ? 0 : 16;
	 		dsubimage(entX, entY, &img_player, entSubrectX, entSubrectY, player.props.width + 4, player.props.height + 1, DIMAGE_NONE);
		}
		else
		{
//			Render top half of swing and bottom half of whatever animation would be playing otherwise
			entSubrectX = player.anim.direction ? 16 : 0;
			dsubimage(entX, entY + 15, &img_player, entSubrectX, entSubrectY + 15, player.props.width + 4, player.props.height - 14, DIMAGE_NONE);

			entSubrectY = (4 - (player.swingFrame >> 3)) * (player.props.height + 2) + 1;
			dsubimage(entX, entY, &img_player, entSubrectX, entSubrectY, player.props.width + 4, 15, DIMAGE_NONE);

//			Get the appropriate swing sprite
			if(player.tool.type == TOOL_TYPE_PICK) swingSprite = player.tool.data.pickData.swingSprite;
			else if(player.tool.type == TOOL_TYPE_SWORD) swingSprite = player.tool.data.swordData.swingSprite;
			else swingSprite = NULL;

//			Might have to generalise for different sized sprites
			entX += (player.props.width >> 1) + (player.anim.direction ? -12 : 0);
			entX += swingHandleDeltaPositions[3 - (player.swingFrame >> 3)][0] * (player.anim.direction ? -1 : 1);
			entY += swingHandleDeltaPositions[3 - (player.swingFrame >> 3)][1];

			entSubrectX = (3 - (player.swingFrame >> 3)) * 17 + 1;
			entSubrectY = player.anim.direction ? 17 : 0;

			dsubimage(entX, entY, swingSprite, entSubrectX, entSubrectY, 16, 16, DIMAGE_NONE);
		}

		if(renderHUD) dimage(player.cursor.x - 2, player.cursor.y - 2, &img_ui_cursor);
	}

//	Render all the particles in the world explosion
	for(int i = 0; i < world.explosion.numParticles; i++)
	{
		particle = world.explosion.particles[i];
		dpixel(particle.x - (camX - (SCREEN_WIDTH >> 1)), particle.y - (camY - (SCREEN_HEIGHT >> 1)), particle.colour);
	}

//	Render the hotbar if the player has recently interacted with their inventory
	if(renderHUD && player.inventory.ticksSinceInteracted < 120)
	{
		if(player.cursor.x < 82 && player.cursor.y < 19) hotbarY = 47;
		else hotbarY = 0;
		dsubimage(0, hotbarY, &img_ui_slots, 0, 0, 80, 17, DIMAGE_NOCLIP);
		dimage(16 * player.inventory.hotbarSlot, hotbarY, &img_ui_slot_highlight);
		for(int slot = 0; slot < 5; slot++)
		{
			item = player.inventory.items[slot];
			if(item.id != ITEM_NULL) renderItem(16 * slot + 1, hotbarY + 1, &item);
		}
	}

//	Render breath meter
	if(player.breath > 0 && player.breath < 200)
	{
		for(int bubble = 0; bubble < player.breath / 40 + 1; bubble++)
		{
			x = player.props.x - (camX - (SCREEN_WIDTH >> 1)) - 9 + bubble * 6;
			y = player.props.y - (camY - (SCREEN_HEIGHT >> 1)) - 6;
			dimage(x, y, &img_ui_bubble);
		}
	}

//	Render the various HUD items
	if(renderHUD)
	{
		dprint_opt(128, 0, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "%i HP", player.combat.health);

		getTime(&hour, &minute);
		dprint_opt(128, 6, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "%02d:%02d", hour, minute);

		if(player.combat.health <= 0)
		{
			dimage(32, 26, &img_ui_deathtext);
		}
	}
}

void takeVRAMCapture()
{
	uint32_t *light;
	uint32_t *dark;
	uint16_t *path = u"\\\\fls0\\capt.vram";
	int descriptor;
	int size = 2048;

	BFile_Remove(path);
	BFile_Create(path, BFile_File, &size);
	descriptor = BFile_Open(path, BFile_WriteOnly);
	dgray_getvram(&light, &dark);
	BFile_Write(descriptor, light, 1024);
	BFile_Write(descriptor, dark, 1024);
	BFile_Close(descriptor);
}

void resetExplosion(int x, int y)
{
	world.explosion.deltaTicks = 0;

	for(int i = 0; i < world.explosion.numParticles; i++)
	{
		world.explosion.particles[i] = (Particle) {
			x,
			y,
			(float)((rand() % 201) - 80) / 100.0,
			(float)((rand() % 201) + 100) / -100.0,
			(rand() % 2) ? C_BLACK : C_LIGHT
		};
	}
}

void updateExplosion()
{
	Particle* particle;
	
	for(int i = 0; i < world.explosion.numParticles; i++)
	{
		particle = &world.explosion.particles[i];

		particle->x += round(particle->xVel);
		particle->y += round(particle->yVel);

		particle->yVel += 0.2;
		particle->xVel *= 0.95;
	}
	world.explosion.deltaTicks++;
}

void middleText(char *text, int progress)
{
	extern bopti_image_t img_generate, img_ui_loadbar;

	dclear(C_WHITE);
	dsubimage(0, 47, &img_generate, 0, 0, 21, 17, DIMAGE_NONE);
	dsubimage(107, 47, &img_generate, 22, 0, 21, 17, DIMAGE_NONE);
	dsubimage(50, 0, &img_generate, 44, 0, 26, 17, DIMAGE_NONE);
	dtext_opt(64, 32, C_BLACK, C_WHITE, DTEXT_CENTER, DTEXT_CENTER, text);
	dsubimage(11, 40, &img_ui_loadbar, 0, 0, 4, 8, DIMAGE_NONE);
	dsubimage(113, 40, &img_ui_loadbar, 5, 0, 4, 8, DIMAGE_NONE);
	drect(14, 42, 14 + progress, 45, C_BLACK);
	dupdate();
}

void setVar(int x, int y)
{
	int virtX = x - varBufferPos.x;
	int virtY = y - varBufferPos.y;
	int dY = 0;
	int var = 0;
	Tile tile = getTile(x, y);

//	Catch any sneaky out-of-buffer variant change requests
	if(virtX < 0 || virtX >= VAR_BUF_WIDTH || virtY < 0 || virtY >= VAR_BUF_HEIGHT) return;

	switch(tile.id)
	{
//		Ugly but optimised

		case TILE_WBENCH_R:
		case TILE_ANVIL_R:
		case TILE_CACTUS_BRANCH:
			var = 1;
		case TILE_WBENCH_L:
		case TILE_ANVIL_L:
		case TILE_CACTUS:
//			Little trick to render branch connectors only if it's the bottom of a branch
			if(tile.id == TILE_CACTUS && getTile(x - 1, y + 1).id != TILE_CACTUS_BRANCH && getTile(x + 1, y + 1).id != TILE_CACTUS_BRANCH) var = 2;
			varBuffer[virtY][virtX] = var;
			break;
		
		case TILE_FURNACE_EDGE:
			if(x == game.WORLD_WIDTH - 1 || getTile(x + 1 , y).id != TILE_FURNACE_MID) var += 2;
		case TILE_CHAIR_L:
			if(tile.id == TILE_CHAIR_L) var += 2;
		case TILE_CHEST_R:
			if(tile.id == TILE_CHEST_R) var += 2;
		case TILE_CRYST_R:
			if(tile.id == TILE_CRYST_R) var += 2;
		case TILE_CHAIR_R:
		case TILE_FURNACE_MID:
		case TILE_CHEST_L:
		case TILE_DOOR_C:
		case TILE_CRYST_L:
		case TILE_DOOR_O_L_L: case TILE_DOOR_O_L_R:
		case TILE_DOOR_O_R_L: case TILE_DOOR_O_R_R:
		while(y - dY - 1 >= 0 && getTile(x, y - dY - 1).id == tile.id) dY++;
		var += dY;
		varBuffer[virtY][virtX] = var;
		break;

		case TILE_VINE:
			varBuffer[virtY][virtX] = rand() % 4;
			break;

		case TILE_PLANT:
			varBuffer[virtY][virtX] = rand() % 10;
			break;

		default:
			varBuffer[virtY][virtX] = makeVar();
			break;
	}
}

void fillVarBuffer(int startX, int startY, int width, int height)
{
	for(int dY = startY; dY < startY + height; dY++)
	{
		for(int dX = startX; dX < startX + width; dX++) setVar(varBufferPos.x + dX, varBufferPos.y + dY);
	}
}

void updateVarBuffer(int x, int y)
{
	int offsetX = x - (varBufferPos.x + VAR_BUF_OFFSET);
	int offsetY = y - (varBufferPos.y + VAR_BUF_OFFSET);
	char *source, *dest;
	int length;

	varBufferPos.x = x - VAR_BUF_OFFSET;
	varBufferPos.y = y - VAR_BUF_OFFSET;

	if(abs(offsetY) >= VAR_BUF_HEIGHT || abs(offsetX) >= VAR_BUF_WIDTH) fillVarBuffer(0, 0, VAR_BUF_WIDTH, VAR_BUF_HEIGHT);
	else
	{
		if(offsetY != 0)
		{
			source = offsetY > 0 ? varBuffer[offsetY] : (char *)varBuffer;
			dest = offsetY > 0 ? (char *)varBuffer : varBuffer[-offsetY];
			length = (VAR_BUF_HEIGHT - abs(offsetY)) * VAR_BUF_WIDTH;
			memmove(dest, source, length);

		}
		if(offsetX != 0)
		{
			for(int dY = 0; dY < VAR_BUF_HEIGHT; dY++)
			{
				source = varBuffer[dY] + (offsetX > 0 ? offsetX : 0);
				dest = varBuffer[dY] + (offsetX > 0 ? 0 : -offsetX);
				length = VAR_BUF_WIDTH - abs(offsetX);
				memmove(dest, source, length);
			}
		}
		if(offsetY != 0)
		{
			fillVarBuffer(
				0,
				offsetY > 0 ? VAR_BUF_HEIGHT - offsetY - 1 : 0,
				VAR_BUF_WIDTH,
				abs(offsetY)
			);
		}
		if(offsetX != 0)
		{
			fillVarBuffer(
				offsetX > 0 ? VAR_BUF_WIDTH - offsetX - 1 : 0,
				0,
				abs(offsetX),
				VAR_BUF_HEIGHT
			);
		}
	}
}
