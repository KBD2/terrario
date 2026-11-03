#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/keyboard.h>

#include "save.h"
#include "defs.h"
#include "world.h"
#include "npc.h"
#include "generate.h"
#include "render.h"

#define CHUNK_MAGIC 0x4B484354u
#define CHUNK_FORMAT_VERSION 1

struct SaveInfo {
	char version[VERSION_BUFFER_SIZE];
	int time;
	int chunkX;
	int chunkY;
	unsigned int worldSeed;
};

struct PlayerSave {
	Item items[INVENTORY_SIZE];
	Item accessories[5];
	Item armour[3];
	int health;
	int maxHealth;
	Coords spawn;
	int spawnChunkX;
	int spawnChunkY;
	int x, y;
};

struct HousingSave {
	Coords position;
	enum NPCs npc;
};

struct ChunkHeader {
	uint32_t magic;
	uint16_t formatVersion;
	uint16_t width;
	uint16_t height;
	uint16_t reserved;
	int chunkX;
	int chunkY;
	unsigned int worldSeed;
	int numChests;
	int numMarkers;
	int numNPCs;
	int numHomeless;
	int dataSize;
};

/* The CASIOWIN filesystem only accepts even-sized writes. */
_Static_assert(sizeof(struct SaveInfo) % 2 == 0, "save.info must be even-sized");
_Static_assert(sizeof(struct PlayerSave) % 2 == 0, "player.dat must be even-sized");
_Static_assert(sizeof(struct ChunkHeader) % 2 == 0, "chunk header must be even-sized");
_Static_assert(sizeof(struct Chest) % 2 == 0, "chest data must be even-sized");
_Static_assert(sizeof(struct HousingSave) % 2 == 0, "housing data must be even-sized");

static void sanitizeItem(Item *item)
{
	if(item->id < 0 || item->id >= ITEMS_COUNT)
	{
		*item = (Item){ITEM_NULL, PREFIX_NONE, 0};
		return;
	}
	if(item->prefix < 0 || item->prefix >= PREFIX_COUNT) item->prefix = PREFIX_NONE;
	if(item->amount < 0) item->amount = 0;
	if(item->id == ITEM_NULL) item->amount = 0;
}

static void toPath(const char *source, uint16_t *destination)
{
	int i;

	for(i = 0; source[i] != '\0' && i < 63; i++)
	{
		destination[i] = (uint16_t)(unsigned char)source[i];
	}
	destination[i] = 0;
}

static void chunkPath(int chunkX, int chunkY, uint16_t *path)
{
	char buffer[64];

	sprintf(buffer, "\\\\fls0\\TERRARIO\\c%i_%i.dat", chunkX, chunkY);
	toPath(buffer, path);
}

// Checks if \\fls0\TERRARIO\save.info exists
bool getSave()
{
	int handle;
	uint16_t foundPath[64];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst((const uint16_t*)u"\\\\fls0\\TERRARIO\\save.info", &handle, foundPath, &fileInfo);
	if(error == 0) BFile_FindClose(handle);

	return error == 0;
}

void getVersionInfo()
{
	extern char versionBuffer[16];
	struct SaveInfo info;
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	int descriptor = BFile_Open(infoPath, BFile_ReadOnly);

	memset(&info, 0, sizeof(info));
	if(descriptor >= 0)
	{
		BFile_Read(descriptor, &info, sizeof(info), 0);
		BFile_Close(descriptor);
	}
	strncpy(versionBuffer, info.version, VERSION_BUFFER_SIZE - 1);
	versionBuffer[VERSION_BUFFER_SIZE - 1] = '\0';
}

static bool ensureSaveFolder()
{
	int handle;
	uint16_t foundPath[64];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst((const uint16_t*)u"\\\\fls0\\TERRARIO", &handle, foundPath, &fileInfo);
	if(error == 0) BFile_FindClose(handle);
	if(error < 0)
	{
		error = BFile_Create((const uint16_t*)u"\\\\fls0\\TERRARIO", BFile_Folder, NULL);
		if(error < 0) return false;
	}
	return true;
}

/* writeWholeFile
Writes a small structure (save.info/player.dat) to a freshly created file. All
sizes used here are even, as required by the CASIOWIN filesystem. */
static bool writeWholeFile(const uint16_t *path, const void *data, int size)
{
	int fileSize = size;
	int descriptor;

	if(size & 1) return false;

	BFile_Remove(path);
	if(BFile_Create(path, BFile_File, &fileSize) < 0) return false;
	descriptor = BFile_Open(path, BFile_WriteOnly);
	if(descriptor < 0) return false;
	if(BFile_Write(descriptor, data, size) < 0)
	{
		BFile_Close(descriptor);
		return false;
	}
	BFile_Close(descriptor);
	return true;
}

/* writeChunkFile
Stores the loaded chunk: tiles, chests, house markers and NPCs. */
static bool writeChunkFile()
{
	uint16_t path[64];
	struct ChunkHeader header;
	int numMarkers = world.numMarkers;
	int numHomeless = 0;
	int numChests = world.chests.number;
	int tileBytes = game.WORLD_WIDTH * game.WORLD_HEIGHT * sizeof(Tile);
	int chestBytes;
	int markerBytes;
	int fileSize;
	int descriptor;
	int offset;
	NPC *npc;
	HouseMarker *marker;
	struct HousingSave housingSave;

	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		if(world.npcs[idx].marker == -1) numHomeless++;
	}

	chestBytes = numChests * sizeof(struct Chest);
	markerBytes = (numMarkers + numHomeless) * sizeof(struct HousingSave);
	fileSize = sizeof(header) + tileBytes + chestBytes + markerBytes;
	if(fileSize & 1) fileSize += 2;

	memset(&header, 0, sizeof(header));
	header.magic = CHUNK_MAGIC;
	header.formatVersion = CHUNK_FORMAT_VERSION;
	header.width = game.WORLD_WIDTH;
	header.height = game.WORLD_HEIGHT;
	header.chunkX = save.chunkX;
	header.chunkY = save.chunkY;
	header.worldSeed = save.worldSeed;
	header.numChests = numChests;
	header.numMarkers = numMarkers;
	header.numNPCs = world.numNPCs;
	header.numHomeless = numHomeless;
	header.dataSize = fileSize;

	chunkPath(save.chunkX, save.chunkY, path);
	BFile_Remove(path);
	if(BFile_Create(path, BFile_File, &fileSize) < 0) return false;
	descriptor = BFile_Open(path, BFile_WriteOnly);
	if(descriptor < 0) return false;

	offset = 0;
	if(BFile_Write(descriptor, &header, sizeof(header)) < 0) goto fail;
	offset += sizeof(header);
	if(BFile_Write(descriptor, world.tiles, tileBytes) < 0) goto fail;
	offset += tileBytes;
	if(chestBytes > 0)
	{
		if(BFile_Write(descriptor, world.chests.chests, chestBytes) < 0) goto fail;
		offset += chestBytes;
	}

	for(int idx = 0; idx < numMarkers; idx++)
	{
		marker = &world.markers[idx];
		housingSave = (struct HousingSave) {
			.position = marker->position,
			.npc = (marker->occupant >= 0 && marker->occupant < world.numNPCs)
				? world.npcs[marker->occupant].id : NPC_NONE
		};
		if(BFile_Write(descriptor, &housingSave, sizeof(housingSave)) < 0) goto fail;
		offset += sizeof(housingSave);
	}
	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		npc = &world.npcs[idx];
		if(npc->marker == -1)
		{
			housingSave = (struct HousingSave) {
				.position = (Coords){npc->props.x, npc->props.y},
				.npc = npc->id
			};
			if(BFile_Write(descriptor, &housingSave, sizeof(housingSave)) < 0) goto fail;
			offset += sizeof(housingSave);
		}
	}

	if(offset < fileSize)
	{
		char padding[2] = {0, 0};
		if(BFile_Write(descriptor, padding, fileSize - offset) < 0) goto fail;
	}

	BFile_Close(descriptor);
	return true;

fail:
	BFile_Close(descriptor);
	return false;
}

void saveGame()
{
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	const uint16_t *playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	struct SaveInfo info;
	struct PlayerSave playerSave;

	save.error = -99;
	save.timeTicks = world.timeTicks;

	if(!ensureSaveFolder())
	{
		save.error = -2;
		return;
	}

//	Write the chunk first: if anything goes wrong the old save.info still
//	points at a consistent chunk.
	if(!writeChunkFile())
	{
		save.error = -6;
		return;
	}

	memset(&playerSave, 0, sizeof(playerSave));
	playerSave.health = player.combat.health;
	playerSave.maxHealth = player.maxHealth;
	memcpy(playerSave.items, player.inventory.items, INVENTORY_SIZE * sizeof(Item));
	memcpy(playerSave.accessories, player.inventory.accessories, 5 * sizeof(Item));
	memcpy(playerSave.armour, player.inventory.armour, 3 * sizeof(Item));
	playerSave.spawn = player.spawn;
	playerSave.spawnChunkX = player.spawnChunkX;
	playerSave.spawnChunkY = player.spawnChunkY;
	playerSave.x = player.props.x;
	playerSave.y = player.props.y;
	if(!writeWholeFile(playerPath, &playerSave, sizeof(playerSave)))
	{
		save.error = -1;
		return;
	}

	memset(&info, 0, sizeof(info));
	strncpy(info.version, VERSION, VERSION_BUFFER_SIZE - 1);
	info.time = save.timeTicks;
	info.chunkX = save.chunkX;
	info.chunkY = save.chunkY;
	info.worldSeed = save.worldSeed;
	if(!writeWholeFile(infoPath, &info, sizeof(info))) save.error = -2;
}

/* clearChunk
Drops all per-chunk state before another chunk is loaded or generated. */
static void clearChunk()
{
	world.chests.number = 0;
	for(int idx = 0; idx < MAX_ENTITIES; idx++) world.entities[idx] = (Entity){ -1 };
	world.explosion.numParticles = 0;
	world.explosion.deltaTicks = 0;

	world.numNPCs = 0;
	free(world.npcs);
	world.npcs = NULL;

	world.numMarkers = 0;
	free(world.markers);
	world.markers = NULL;
}

void loadChunkFile()
{
	uint16_t path[64];
	struct ChunkHeader header;
	struct HousingSave housingSave;
	NPC *npc;
	HouseMarker *marker;
	int descriptor;
	int fileSize;
	int tileBytes = game.WORLD_WIDTH * game.WORLD_HEIGHT * sizeof(Tile);
	int chestBytes;
	int markerBytes;
	int offset;

	save.chunkMissing = false;
	chunkPath(save.chunkX, save.chunkY, path);

	descriptor = BFile_Open(path, BFile_ReadOnly);
	if(descriptor < 0)
	{
		save.chunkMissing = true;
		save.error = -99;
		return;
	}

	fileSize = BFile_Size(descriptor);
	if(fileSize < (int)sizeof(header) || BFile_Read(descriptor, &header, sizeof(header), 0) < 0)
	{
		BFile_Close(descriptor);
		save.error = -4;
		return;
	}

	if(header.magic != CHUNK_MAGIC
	|| header.formatVersion != CHUNK_FORMAT_VERSION
	|| header.width != game.WORLD_WIDTH
	|| header.height != game.WORLD_HEIGHT
	|| header.chunkX != save.chunkX
	|| header.chunkY != save.chunkY
	|| header.dataSize != fileSize
	|| header.numChests < 0 || header.numChests > MAX_CHESTS
	|| header.numMarkers < 0 || header.numMarkers > MAX_HOUSE_MARKERS
	|| header.numNPCs < 0 || header.numNPCs > MAX_NPCS
	|| header.numHomeless < 0 || header.numHomeless > MAX_NPCS)
	{
		BFile_Close(descriptor);
		save.error = -4;
		return;
	}

	/* A chunk file left over from another world can share these coordinates.
	   Treat it as absent and let the caller generate this world's version. */
	if(header.worldSeed != save.worldSeed)
	{
		BFile_Close(descriptor);
		save.chunkMissing = true;
		save.error = -99;
		return;
	}

	chestBytes = header.numChests * sizeof(struct Chest);
	markerBytes = (header.numMarkers + header.numHomeless) * sizeof(struct HousingSave);
	if((int)(sizeof(header) + tileBytes + chestBytes + markerBytes) != fileSize)
	{
		BFile_Close(descriptor);
		save.error = -4;
		return;
	}

	clearChunk();

	offset = sizeof(header);
	if(BFile_Read(descriptor, world.tiles, tileBytes, offset) < 0) goto fail;
	offset += tileBytes;
	for(int idx = 0; idx < game.WORLD_WIDTH * game.WORLD_HEIGHT; idx++)
	{
		if(world.tiles[idx].id >= TILES_COUNT) world.tiles[idx] = (Tile){TILE_STONE};
	}
	world.chests.number = header.numChests;
	if(chestBytes > 0 && BFile_Read(descriptor, world.chests.chests, chestBytes, offset) < 0) goto fail;
	offset += chestBytes;
	for(int chest = 0; chest < header.numChests; chest++)
	{
		for(int slot = 0; slot < INVENTORY_SIZE; slot++)
		{
			sanitizeItem(&world.chests.chests[chest].items[slot]);
		}
	}

	for(int idx = 0; idx < header.numMarkers; idx++)
	{
		int numNPCs;
		int numMarkers;

		if(BFile_Read(descriptor, &housingSave, sizeof(housingSave), offset) < 0) goto fail;
		offset += sizeof(housingSave);
		if(housingSave.npc < NPC_NONE || housingSave.npc > NPC_NURSE) housingSave.npc = NPC_NONE;
		if(!checkHousingValid(housingSave.position))
		{
			if(housingSave.npc != NPC_NONE)
			{
				numNPCs = world.numNPCs;
				addNPC(housingSave.npc);
				if(world.numNPCs == numNPCs) continue;
				npc = &world.npcs[world.numNPCs - 1];
				npc->props.x = housingSave.position.x << 3;
				npc->props.y = housingSave.position.y << 3;
				continue;
			}
		}
		numMarkers = world.numMarkers;
		addMarker(housingSave.position);
		if(world.numMarkers == numMarkers) continue;
		if(housingSave.npc != NPC_NONE)
		{
			numNPCs = world.numNPCs;
			addNPC(housingSave.npc);
			if(world.numNPCs == numNPCs) continue;
			marker = &world.markers[world.numMarkers - 1];
			npc = &world.npcs[world.numNPCs - 1];
			npc->props.x = housingSave.position.x << 3;
			npc->props.y = housingSave.position.y << 3;
			npc->marker = world.numMarkers - 1;
			marker->occupant = world.numNPCs - 1;
		}
	}
	for(int idx = 0; idx < header.numHomeless; idx++)
	{
		int numNPCs;

		if(BFile_Read(descriptor, &housingSave, sizeof(housingSave), offset) < 0) goto fail;
		offset += sizeof(housingSave);
		if(housingSave.npc < NPC_NONE || housingSave.npc > NPC_NURSE) housingSave.npc = NPC_NONE;
		numNPCs = world.numNPCs;
		addNPC(housingSave.npc);
		if(world.numNPCs == numNPCs) continue;
		npc = &world.npcs[world.numNPCs - 1];
		npc->props.x = housingSave.position.x;
		npc->props.y = housingSave.position.y;
	}

	BFile_Close(descriptor);
	save.error = -99;
	return;

fail:
	BFile_Close(descriptor);
	save.error = -4;
}

void loadSave()
{
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	const uint16_t *playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	struct SaveInfo info;
	struct PlayerSave playerSave;
	int descriptor;
	int size;

	save.error = -99;

	descriptor = BFile_Open(infoPath, BFile_ReadOnly);
	if(descriptor < 0)
	{
		save.error = -2;
		return;
	}
	size = BFile_Size(descriptor);
	if(size != (int)sizeof(info))
	{
		BFile_Close(descriptor);
		save.error = -5;
		return;
	}
	if(BFile_Read(descriptor, &info, sizeof(info), 0) < 0)
	{
		BFile_Close(descriptor);
		save.error = -5;
		return;
	}
	BFile_Close(descriptor);

	if(strncmp(info.version, VERSION, VERSION_BUFFER_SIZE) != 0)
	{
		save.error = -5;
		return;
	}

	descriptor = BFile_Open(playerPath, BFile_ReadOnly);
	if(descriptor < 0)
	{
		save.error = -1;
		return;
	}
	size = BFile_Size(descriptor);
	if(size != (int)sizeof(playerSave))
	{
		BFile_Close(descriptor);
		save.error = -5;
		return;
	}
	if(BFile_Read(descriptor, &playerSave, sizeof(playerSave), 0) < 0)
	{
		BFile_Close(descriptor);
		save.error = -1;
		return;
	}
	BFile_Close(descriptor);

	save.timeTicks = info.time;
	save.chunkX = info.chunkX;
	save.chunkY = info.chunkY;
	save.worldSeed = info.worldSeed;
	world.timeTicks = save.timeTicks;

	player.maxHealth = playerSave.maxHealth > 0 ? playerSave.maxHealth : 100;
	player.combat.health = playerSave.health;
	if(player.combat.health < 0) player.combat.health = 0;
	if(player.combat.health > player.maxHealth) player.combat.health = player.maxHealth;
	memcpy(player.inventory.items, playerSave.items, INVENTORY_SIZE * sizeof(Item));
	memcpy(player.inventory.accessories, playerSave.accessories, 5 * sizeof(Item));
	memcpy(player.inventory.armour, playerSave.armour, 3 * sizeof(Item));
	for(int slot = 0; slot < INVENTORY_SIZE; slot++) sanitizeItem(&player.inventory.items[slot]);
	for(int slot = 0; slot < 5; slot++) sanitizeItem(&player.inventory.accessories[slot]);
	for(int slot = 0; slot < 3; slot++) sanitizeItem(&player.inventory.armour[slot]);
	player.spawn = playerSave.spawn;
	player.spawnChunkX = playerSave.spawnChunkX;
	player.spawnChunkY = playerSave.spawnChunkY;

	loadChunkFile();
	if(save.chunkMissing) save.error = -3;
	if(save.error != -99) return;

	player.props.x = playerSave.x;
	player.props.y = playerSave.y;
}

/* carvePlayerSpace
Makes sure the player is not embedded in solid ground after entering a chunk
through an edge, where the stitched terrain may not leave a doorway. */
static void carvePlayerSpace()
{
	int left = player.props.x >> 3;
	int right = (player.props.x + player.props.width - 1) >> 3;
	int top = player.props.y >> 3;
	int bottom = (player.props.y + player.props.height - 1) >> 3;

	for(int y = top; y <= bottom; y++)
	{
		for(int x = left; x <= right; x++)
		{
			enum PhysicsTypes physics = tiles[getTile(x, y).id].physics;
			if(physics == PHYS_SOLID || physics == PHYS_SAND) setTile(x, y, TILE_NOTHING);
		}
	}
}

bool travelToChunk(int chunkX, int chunkY, int playerX, int playerY)
{
	int oldChunkX = save.chunkX;
	int oldChunkY = save.chunkY;
	int failure;

	if(chunkX == save.chunkX && chunkY == save.chunkY)
	{
		player.props.x = playerX;
		player.props.y = playerY;
		return true;
	}

	gint_world_switch(GINT_CALL(&saveGame));
	if(save.error != -99) return false;

	save.chunkX = chunkX;
	save.chunkY = chunkY;
	save.chunkMissing = false;

	gint_world_switch(GINT_CALL(&loadChunkFile));
	if(save.chunkMissing)
	{
		clearChunk();
		generateWorld();
		save.error = -99;
	}
	else if(save.error != -99)
	{
//		The target chunk is damaged; put the saved chunk back in memory
		failure = save.error;
		save.chunkX = oldChunkX;
		save.chunkY = oldChunkY;
		save.chunkMissing = false;
		gint_world_switch(GINT_CALL(&loadChunkFile));
		save.error = failure;
		return false;
	}

	save.error = -99;
	save.chunkMissing = false;
	player.props.x = playerX;
	player.props.y = playerY;
	player.props.xVel = 0;
	player.props.yVel = 0;
	player.props.touchingTileTop = false;
	player.props.dropping = false;
	player.props.movingSelf = false;
	player.jumpTimer = 0;
	player.jumpReleased = true;
	player.pixelsFallen = 0;

	carvePlayerSpace();
	resetVarBuffer();
	clearevents();
	return true;
}
