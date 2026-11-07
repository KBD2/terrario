#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <gint/bfile.h>
#include <gint/gint.h>

#include "chest.h"
#include "defs.h"
#include "save.h"
#include "world.h"
#include "stream.h"
#include "worldgen.h"

#define CHEST_FILE_MAGIC 0x54534548u
#define CHEST_FILE_VERSION 2

struct ChestFileHeader {
	uint32_t magic;
	uint16_t version;
	uint16_t reserved;
	uint32_t worldSeed;
	int32_t topX;
	int32_t topY;
};

_Static_assert(sizeof(struct ChestFileHeader) % 2 == 0, "chest header must be even-sized");
_Static_assert(sizeof(struct Chest) % 2 == 0, "chest data must be even-sized");

enum ChestReadResult {
	CHEST_READ_OK = 1,
	CHEST_READ_MISSING = 2,
	CHEST_READ_BAD = 3
};

/* chestPath
One file per chest, named K + 4 base-32 digits of the global X of its top-left
tile + 3 digits of its global Y. The header also carries the world seed, so
files from another world are ignored. */
static void chestPath(int x, int y, uint16_t *path)
{
	char name[16];

	name[0] = 'K';
	encodeName(name + 1, (unsigned)(x + WORLD_LIMIT_X), 4);
	encodeName(name + 5, (unsigned)(y + WORLD_LIMIT_Y), 3);
	strcpy(name + 8, ".dat");
	savePath(name, path);
}

static void sanitizeChest(struct Chest *chest)
{
	for(int slot = 0; slot < INVENTORY_SIZE; slot++)
	{
		Item *item = &chest->items[slot];

		if(item->id < 0 || item->id >= ITEMS_COUNT)
		{
			*item = (Item){ITEM_NULL, PREFIX_NONE, 0};
			continue;
		}
		if(item->prefix < 0 || item->prefix >= PREFIX_COUNT) item->prefix = PREFIX_NONE;
		if(item->amount < 0) item->amount = 0;
		if(item->id == ITEM_NULL) item->amount = 0;
	}
}

/* All the file operations below take global tile coordinates */

static int writeChestRecord(const struct Chest *chest)
{
	uint16_t path[48];
	struct ChestFileHeader header;
	int fileSize = sizeof(header) + sizeof(struct Chest);
	int descriptor;

	if(!ensureSaveFolder()) return 0;

	chestPath(chest->topX, chest->topY, path);
	BFile_Remove(path);
	if(BFile_Create(path, BFile_File, &fileSize) < 0) return 0;
	descriptor = BFile_Open(path, BFile_WriteOnly);
	if(descriptor < 0) return 0;

	memset(&header, 0, sizeof(header));
	header.magic = CHEST_FILE_MAGIC;
	header.version = CHEST_FILE_VERSION;
	header.worldSeed = save.worldSeed;
	header.topX = chest->topX;
	header.topY = chest->topY;

	if(BFile_Write(descriptor, &header, sizeof(header)) < 0
	|| BFile_Write(descriptor, chest, sizeof(struct Chest)) < 0)
	{
		BFile_Close(descriptor);
		return 0;
	}

	BFile_Close(descriptor);
	return 1;
}

static int createChestOp(int x, int y)
{
	struct Chest chest;

	memset(&chest, 0, sizeof(chest));
	chest.topX = x;
	chest.topY = y;
	return writeChestRecord(&chest);
}

static int readChestOp(int x, int y)
{
	uint16_t path[48];
	struct ChestFileHeader header;
	struct Chest chest;
	int descriptor;

	chestPath(x, y, path);
	descriptor = BFile_Open(path, BFile_ReadOnly);
	if(descriptor < 0) return CHEST_READ_MISSING;
	if(BFile_Size(descriptor) != (int)(sizeof(header) + sizeof(chest)))
	{
		BFile_Close(descriptor);
		return CHEST_READ_BAD;
	}
	if(BFile_Read(descriptor, &header, sizeof(header), 0) < 0
	|| BFile_Read(descriptor, &chest, sizeof(chest), sizeof(header)) < 0)
	{
		BFile_Close(descriptor);
		return CHEST_READ_BAD;
	}
	BFile_Close(descriptor);

//	A leftover from an older world counts as no file at all
	if(header.magic != CHEST_FILE_MAGIC
	|| header.version != CHEST_FILE_VERSION
	|| header.worldSeed != save.worldSeed) return CHEST_READ_MISSING;

	if(header.topX != x || header.topY != y || chest.topX != x || chest.topY != y)
		return CHEST_READ_BAD;

	sanitizeChest(&chest);
	world.chests.active = chest;
	return CHEST_READ_OK;
}

static int writeChestOp()
{
	if(!world.chests.activeValid) return 0;
	return writeChestRecord(&world.chests.active);
}

static int removeChestOp(int x, int y)
{
	uint16_t path[48];
	struct ChestFileHeader header;
	int descriptor;

	chestPath(x, y, path);
	descriptor = BFile_Open(path, BFile_ReadOnly);
	if(descriptor < 0) return 0;
	if(BFile_Size(descriptor) != (int)(sizeof(header) + sizeof(struct Chest))
	|| BFile_Read(descriptor, &header, sizeof(header), 0) < 0)
	{
		BFile_Close(descriptor);
		return 0;
	}
	BFile_Close(descriptor);

	if(header.magic != CHEST_FILE_MAGIC
	|| header.version != CHEST_FILE_VERSION
	|| header.worldSeed != save.worldSeed
	|| header.topX != x
	|| header.topY != y)
	{
		return 0;
	}

	BFile_Remove(path);
	return 1;
}

/* The public functions take window-local tile coordinates, like the rest of
   the game, and convert them to global ones. */

bool addChest(int x, int y)
{
	int gx = x + stream.originX, gy = y + stream.originY;
	struct Chest empty;

	if(world.chests.activeValid && world.chests.active.topX == gx && world.chests.active.topY == gy) return true;
	if(world.chests.activeValid && !flushChest()) return false;

	if(gint_world_switch(GINT_CALL(&createChestOp, gx, gy)) != 1) return false;

	memset(&empty, 0, sizeof(empty));
	empty.topX = gx;
	empty.topY = gy;
	world.chests.active = empty;
	world.chests.activeValid = true;
	return true;
}

bool removeChest(int x, int y)
{
	int gx = x + stream.originX, gy = y + stream.originY;

	if(world.chests.activeValid
	&& world.chests.active.topX == gx
	&& world.chests.active.topY == gy)
	{
		world.chests.activeValid = false;
	}

	return gint_world_switch(GINT_CALL(&removeChestOp, gx, gy)) == 1;
}

struct Chest *findChest(int x, int y)
{
	int gx = x + stream.originX, gy = y + stream.originY;
	int result;

	if(world.chests.activeValid
	&& world.chests.active.topX == gx
	&& world.chests.active.topY == gy)
	{
		return &world.chests.active;
	}

	if(world.chests.activeValid && !flushChest()) return NULL;

	world.chests.activeValid = false;
	result = gint_world_switch(GINT_CALL(&readChestOp, gx, gy));
	if(result == CHEST_READ_OK)
	{
		world.chests.activeValid = true;
		return &world.chests.active;
	}
	if(result != CHEST_READ_MISSING) return NULL;

//	No file yet: a chest placed by the world generator. Its loot is rolled
//	now, the first time it is opened, and saved right away.
	if(getTile(x, y).id != TILE_CHEST_L) return NULL;
	memset(&world.chests.active, 0, sizeof(world.chests.active));
	world.chests.active.topX = gx;
	world.chests.active.topY = gy;
	world.chests.activeValid = true;
	addLoot(&world.chests.active, wgChestLoot(gx, gy) == WG_LOOT_UNDERGROUND ? TABLE_UNDERGROUND : TABLE_SURFACE);
	if(!flushChest())
	{
		world.chests.activeValid = false;
		return NULL;
	}
	return &world.chests.active;
}

bool flushChest()
{
	if(!world.chests.activeValid) return true;

	if(gint_world_switch(GINT_CALL(&writeChestOp)) != 1) return false;
	return true;
}

void closeChest()
{
	world.chests.activeValid = false;
}
