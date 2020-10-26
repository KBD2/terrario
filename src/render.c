#include <gint/gray.h>
#include <gint/defs/util.h>
#include <stdbool.h>
#include <gint/bfile.h>
#include <gint/std/string.h>
#include <gint/timer.h>
#include <gint/std/stdlib.h>
#include <gint/std/stdio.h>
#include <math.h>

#include "render.h"
#include "defs.h"
#include "world.h"
#include "entity.h"
#include "inventory.h"
#include "menu.h"

const unsigned int camMinX = SCREEN_WIDTH >> 1;
const unsigned int camMaxX = (WORLD_WIDTH << 3) - (SCREEN_WIDTH >> 1);
const unsigned int camMinY = SCREEN_HEIGHT >> 1;
const unsigned int camMaxY = (WORLD_HEIGHT << 3) - (SCREEN_HEIGHT >> 1);

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
	
	if(items[item->id].maxStack > 1) {
		dprint(x + 1, y + 9, C_BLACK, "%i", item->amount);
		dimage(x + 3, y, items[item->id].sprite);
	}
	else dimage(x + 3, y + 3, items[item->id].sprite);
}

void render()
{
	extern bopti_image_t img_player, img_cursor, img_hotbar, img_hotbarselect,
	img_leaves, img_swing_copper_sword, img_swing_copper_pick, img_deathtext,
	img_bg_underground, img_sunmoon, img_bg_night;
	bopti_image_t *swingSprite;
	int camX = min(max(player.props.x + (player.props.width >> 1), camMinX), camMaxX);
	int camY = min(max(player.props.y + (player.props.height >> 1), camMinY), camMaxY);

//	Translating cam bounds to tile bounds is painful
	unsigned int tileLeftX = max(0, ((camX - (SCREEN_WIDTH >> 1)) >> 3));
	unsigned int tileRightX = min(WORLD_WIDTH - 1, tileLeftX + (SCREEN_WIDTH >> 3));
	unsigned int tileTopY = max(0, ((camY - (SCREEN_HEIGHT >> 1)) >> 3));
	unsigned int tileBottomY = min(WORLD_HEIGHT - 1, tileTopY + (SCREEN_HEIGHT >> 3));

	Tile tile;
	const TileData *currTile;
	unsigned int currTileX, currTileY;
	int camOffsetX = (camX - (SCREEN_WIDTH >> 1));
	int camOffsetY = (camY - (SCREEN_HEIGHT >> 1));
	bool marginLeft, marginRight, marginTop, marginBottom;
	int flags;
	int state;

	int subrectX, subrectY;
	int entX, entY;
	int entSubrectX, entSubrectY;
	Entity *ent;

	char buf[10];
	int width;

	Item item;

	int orbX, orbY;
	float dayPolarAngle;

	int hour, minute;

	player.cursorTile.x = (camX + player.cursor.x - (SCREEN_WIDTH >> 1)) >> 3;
	player.cursorTile.y = (camY + player.cursor.y - (SCREEN_HEIGHT >> 1)) >> 3;

	dclear(C_WHITE);

	if(player.props.y > (WORLD_HEIGHT / 2.8) * 8) dimage(0, 0, &img_bg_underground);
	else
	{
		if(world.timeTicks < timeToTicks(4, 30) || world.timeTicks > timeToTicks(19, 30)) dimage(0, 0, &img_bg_night);

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

	for(unsigned int y = tileTopY; y <= min(tileBottomY + 4, WORLD_HEIGHT - 1); y++)
	{
		for(unsigned int x = max(0, tileLeftX - 2); x <= min(tileRightX + 2, WORLD_WIDTH - 1); x++)
		{
			if(getTile(x, y).id == TILE_LEAVES)
			{
				currTileX = (x << 3) - camOffsetX;
				currTileY = (y << 3) - camOffsetY;
				dsubimage(currTileX - 16, currTileY - 32, &img_leaves, 41 * getTile(x, y).variant + 1, 0, 40, 40, DIMAGE_NONE);
				continue;
			}
		}
	}

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
					if(currTile->spriteType == TYPE_TILE_VAR) subrectX = 9 * tile.variant + 1;
					if(currTile->spriteType == TYPE_SHEET_VAR) subrectX += 37 * tile.variant;
					dsubimage(currTileX, currTileY, currTile->sprite, subrectX, subrectY, 8, 8, flags);
				}
				else
				{
					dsubimage(currTileX, currTileY, currTile->sprite, 0, 0, 8, 8, flags);
				}
			}
		}
	}

	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		if(world.entities[idx].id != -1)
		{
			ent = &world.entities[idx];

			entX = ent->props.x - (camX - (SCREEN_WIDTH >> 1));
			entY = ent->props.y - (camY - (SCREEN_HEIGHT >> 1));
			entSubrectX = ent->anim.direction ? ent->props.width : 0;
			entSubrectY = ent->anim.animationFrame * (ent->props.height + 1) + 1;
			dsubimage(entX, entY, ent->sprite, entSubrectX, entSubrectY, ent->props.width, ent->props.height, DIMAGE_NONE);
		}
	}

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
			entSubrectX = !player.swingDir ? 0 : 16;
			dsubimage(entX, entY + 15, &img_player, entSubrectX, entSubrectY + 15, player.props.width + 4, player.props.height - 14, DIMAGE_NONE);

			entSubrectY = (4 - (player.swingFrame >> 3)) * (player.props.height + 2) + 1;
			dsubimage(entX, entY, &img_player, entSubrectX, entSubrectY, player.props.width + 4, 15, DIMAGE_NONE);

			switch(player.inventory.getSelected()->id)
			{
				case ITEM_COPPER_SWORD:
					swingSprite = &img_swing_copper_sword;
					break;
				
				case ITEM_COPPER_PICK:
					swingSprite = &img_swing_copper_pick;
					break;
				
				default:
					swingSprite = NULL;
					break;
			}
//			Might have to generalise for different sized sprites
			entX += (player.props.width >> 1) + (player.swingDir ? -12 : 0);
			entX += swingHandleDeltaPositions[3 - (player.swingFrame >> 3)][0] * (player.swingDir ? -1 : 1);
			entY += swingHandleDeltaPositions[3 - (player.swingFrame >> 3)][1];

			entSubrectX = (3 - (player.swingFrame >> 3)) * 17 + 1;
			entSubrectY = player.swingDir ? 17 : 0;

			dsubimage(entX, entY, swingSprite, entSubrectX, entSubrectY, 16, 16, DIMAGE_NONE);
		}

		dimage(player.cursor.x - 2, player.cursor.y - 2, &img_cursor);
	}

	if(world.explosion.numParticles > 0)
	{
		renderAndUpdateExplosion(&world.explosion, (camX - (SCREEN_WIDTH >> 1)), (camY - (SCREEN_HEIGHT >> 1)));
		// Only bother rendering 30 frames (60 updates)
		if(world.explosion.deltaTicks == 30) world.explosion.numParticles = 0;
	}

	if(player.inventory.ticksSinceInteracted < 120)
	{
		dimage(0, 0, &img_hotbar);
		dimage(16 * player.inventory.hotbarSlot, 0, &img_hotbarselect);
		for(int slot = 0; slot < 5; slot++)
		{
			item = player.inventory.items[slot];
			if(item.id != ITEM_NULL) renderItem(16 * slot + 1, 1, &item);
		}
	}

	sprintf(buf, "%i HP", player.combat.health);
	dsize(buf, NULL, &width, NULL);
	dtext_opt(128 - width, 0, C_BLACK, C_WHITE, DTEXT_LEFT, DTEXT_TOP, buf);

	getTime(&hour, &minute);
	dprint_opt(128, 6, C_BLACK, C_WHITE, DTEXT_RIGHT, DTEXT_TOP, "%02d:%02d", hour, minute);

	if(player.combat.health <= 0)
	{
		dimage(32, 26, &img_deathtext);
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

void createExplosion(struct ParticleExplosion *explosion, int x, int y)
{
	*explosion = (struct ParticleExplosion) {
		.numParticles = 50,
		.deltaTicks = 0
	};

	for(int i = 0; i < 50; i++)
	{
		explosion->particles[i] = (Particle) {
			x,
			y,
			(float)((rand() % 201) - 80) / 100.0,
			(float)((rand() % 201) + 100) / -100.0
		};
	}
}

void destroyExplosion(struct ParticleExplosion *explosion)
{
	if(explosion->particles != NULL)
	{
		free(explosion->particles);
		explosion->particles = NULL;
	}
	explosion->numParticles = 0;
}

void renderAndUpdateExplosion(struct ParticleExplosion *explosion, int offsetX, int offsetY)
{
	Particle* particle;
	
	for(int i = 0; i < explosion->numParticles; i++)
	{
		particle = &explosion->particles[i];

		dpixel(particle->x - offsetX, particle->y - offsetY, C_BLACK);

		particle->x += round(particle->xVel);
		particle->y += round(particle->yVel);

		particle->yVel += 0.2;
		particle->xVel *= 0.95;
	}
	explosion->deltaTicks++;
}

void middleText(char *text)
{
	extern bopti_image_t img_generate;
	dclear(C_WHITE);
	dsubimage(0, 47, &img_generate, 0, 0, 21, 17, DIMAGE_NONE);
	dsubimage(107, 47, &img_generate, 22, 0, 21, 17, DIMAGE_NONE);
	dsubimage(50, 0, &img_generate, 44, 0, 26, 17, DIMAGE_NONE);
	dtext_opt(64, 32, C_BLACK, C_WHITE, DTEXT_CENTER, DTEXT_CENTER, text);
	dupdate();
}

// Prefer the vram capture as this is original size and not good for the web
/*
void takeScreenshot()
{
	uint32_t *light;
	uint32_t *dark;

	uint16_t *path = u"\\\\fls0\\scrncapt.bmp";
	int descriptor;

	unsigned char *VRAMBuffer = malloc(2048);
	unsigned char *lightBuffer = VRAMBuffer;
	unsigned char *darkBuffer = VRAMBuffer + 1024;

	unsigned char *buffer = malloc(4096 + 70);

	unsigned int width = 128;
	unsigned int height = 64;
	unsigned int dataSize = width * height / 2;
	unsigned int size = dataSize + 70;

	unsigned char header[70] = {
		// All integers are stored as little-endian chars
		0x42, 0x4d,						// Identifier
		(unsigned char)size,			// Filesize
		(unsigned char)(size >> 8),
		(unsigned char)(size >> 16),
		(unsigned char)(size >> 24),
		0x00, 0x00, 0x00, 0x00,			// Not used
		0x46, 0x00, 0x00, 0x00,			// Image data offset
		// DIB
		0x28, 0x00, 0x00, 0x00,			// DIB size
		(unsigned char)width,			// Bitmap width
		(unsigned char)(width >> 8),
		(unsigned char)(width >> 16),
		(unsigned char)(width >> 24),
		(unsigned char)height,			// Bitmap height
		(unsigned char)(height >> 8),
		(unsigned char)(height >> 16),
		(unsigned char)(height >> 24),
		0x01, 0x00,						//.amount of colour planes
		0x04, 0x00,						// BPP
		0x00, 0x00, 0x00, 0x00,			// Compression method
		(unsigned char)dataSize,		// Raw bitmap data size
		(unsigned char)(dataSize >> 8),
		(unsigned char)(dataSize >> 16),
		(unsigned char)(dataSize >> 24),
		0xff, 0x00, 0x00, 0x00,			// Horizontal resolution
		0xff, 0x00, 0x00, 0x00,			// Vertical resolution
		0x04, 0x00, 0x00, 0x00,			//.amount of palette colours
		0x00, 0x00, 0x00, 0x00,			//.amount of important colours
		// Colour Table
		0xff, 0xff, 0xff, 0x00,			// White
		0x88, 0x88, 0x88, 0x00, 		// Light gray
		0x10, 0x10, 0x10, 0x00, 		// Dark gray
		0x00, 0x00, 0x00, 0x00 			// Black
	};
	Pair *imageBuffer = (Pair*)(buffer + 70);
	int place;
	int lightOn;
	int darkOn;
	unsigned char col;
	int sizeInt = size;

	dgray_getvram(&light, &dark);

	memcpy(lightBuffer, light, 1024);
	memcpy(darkBuffer, dark, 1024);

	memcpy(buffer, header, 70);

	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			lightOn = lightBuffer[(y * width + x) >> 3] & (1 << (7 - (x % 8)));
			darkOn = darkBuffer[(y * width + x) >> 3] & (1 << (7 - (x % 8)));
			place = (((height - 1) - y) * width + x) >> 1;

			if (darkOn && lightOn) col = 3;
			else if (darkOn) col = 2;
			else if (lightOn) col = 1;
			else col = 0;

			if (x % 2 == 0) imageBuffer[place].pixel0 = col;
			else imageBuffer[place].pixel1 = col;
		}
	}

	BFile_Remove(path);
	BFile_Create(path, BFile_File, &sizeInt);

	descriptor = BFile_Open(path, BFile_WriteOnly);
	BFile_Write(descriptor, buffer, size);
	BFile_Close(descriptor);

	free(VRAMBuffer);
	free(buffer);
}
*/