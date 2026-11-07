#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gint/bfile.h>
#include <gint/gint.h>

#include "save.h"
#include "defs.h"
#include "world.h"
#include "npc.h"
#include "stream.h"
#include "worldgen.h"

#define NPC_FILE_MAGIC 0x5343504Eu
#define NPC_FILE_VERSION 1
#define MAX_NPC_RECORDS (MAX_HOUSE_MARKERS + MAX_NPCS)

struct SaveInfo {
	char version[VERSION_BUFFER_SIZE];
	int32_t time;
	uint32_t worldSeed;
};

/* All positions are global pixel coordinates */
struct PlayerSave {
	Item items[INVENTORY_SIZE];
	Item accessories[5];
	Item armour[3];
	int32_t health;
	int32_t maxHealth;
	int32_t spawnX, spawnY;
	int32_t x, y;
};

struct NpcFileHeader {
	uint32_t magic;
	uint16_t version;
	uint16_t count;
};

enum NpcRecordTypes {
	RECORD_MARKER,		// x, y: global tile of the marker; npc: occupant or NPC_NONE
	RECORD_HOMELESS		// x, y: global pixel position of an NPC without a house
};

struct NpcRecord {
	int32_t x, y;
	int16_t npc;
	int16_t type;
};

/* The CASIOWIN filesystem only accepts even-sized writes. */
_Static_assert(sizeof(struct SaveInfo) % 2 == 0, "save.info must be even-sized");
_Static_assert(sizeof(struct PlayerSave) % 2 == 0, "player.dat must be even-sized");
_Static_assert(sizeof(struct NpcFileHeader) % 2 == 0, "npc header must be even-sized");
_Static_assert(sizeof(struct NpcRecord) % 2 == 0, "npc record must be even-sized");

/* Player position read by loadSave(), used by finishLoad() */
static int loadedX, loadedY;

void encodeName(char *out, unsigned int value, int digits)
{
	static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

	for(int i = digits - 1; i >= 0; i--)
	{
		out[i] = alphabet[value & 31];
		value >>= 5;
	}
	out[digits] = '\0';
}

void savePath(const char *name, uint16_t *path)
{
	char buffer[48];
	int i;

	snprintf(buffer, sizeof(buffer), "%s%s", SAVE_FOLDER_ASCII, name);
	for(i = 0; buffer[i] != '\0' && i < 47; i++) path[i] = (uint16_t)(unsigned char)buffer[i];
	path[i] = 0;
}

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

bool getSave()
{
	int handle;
	uint16_t foundPath[64];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst(SAVE_FILE("save.info"), &handle, foundPath, &fileInfo);
	if(error == 0) BFile_FindClose(handle);

	return error == 0;
}

void getVersionInfo()
{
	extern char versionBuffer[16];
	struct SaveInfo info;
	int descriptor = BFile_Open(SAVE_FILE("save.info"), BFile_ReadOnly);

	memset(&info, 0, sizeof(info));
	if(descriptor >= 0)
	{
		BFile_Read(descriptor, &info, sizeof(info), 0);
		BFile_Close(descriptor);
	}
	strncpy(versionBuffer, info.version, VERSION_BUFFER_SIZE - 1);
	versionBuffer[VERSION_BUFFER_SIZE - 1] = '\0';
}

bool ensureSaveFolder()
{
	int handle;
	uint16_t foundPath[64];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst(SAVE_FOLDER, &handle, foundPath, &fileInfo);
	if(error == 0) BFile_FindClose(handle);
	if(error < 0)
	{
		error = BFile_Create(SAVE_FOLDER, BFile_Folder, NULL);
		if(error < 0) return false;
	}
	return true;
}

/* Extracts the ASCII file name from a search result, which may or may not
   include the folder. Returns false for names we don't own. */
static bool worldFileName(const uint16_t *found, char *name)
{
	int start = 0, length = 0;

	for(int i = 0; found[i] != 0 && i < 63; i++) if(found[i] == '\\') start = i + 1;
	while(found[start + length] != 0 && length < 12)
	{
		uint16_t c = found[start + length];
		if(c > 127) return false;
		name[length++] = (char)c;
	}
	name[length] = '\0';
	if(found[start + length] != 0 || length < 5) return false;
	if(name[0] != 'C' && name[0] != 'c' && name[0] != 'K' && name[0] != 'k') return false;
	return strcmp(name + length - 4, ".dat") == 0 || strcmp(name + length - 4, ".DAT") == 0;
}

void clearSaveFolder()
{
	char names[16][13];
	uint16_t found[64], path[48];
	struct BFile_FileInfo info;
	int handle, count, removed;

//	Searching while deleting is not safe, so collect a batch, delete it and
//	start a new search until nothing is left
	for(int pass = 0; pass < 200; pass++)
	{
		count = 0;
		if(BFile_FindFirst(SAVE_FILE("*"), &handle, found, &info) != 0) return;
		do
		{
			if(worldFileName(found, names[count])) count++;
		}
		while(count < 16 && BFile_FindNext(handle, found, &info) == 0);
		BFile_FindClose(handle);

		removed = 0;
		for(int i = 0; i < count; i++)
		{
			savePath(names[i], path);
			if(BFile_Remove(path) == 0) removed++;
		}
		if(removed == 0) return;
	}
}

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

static bool writeNpcFile()
{
	struct {
		struct NpcFileHeader header;
		struct NpcRecord records[MAX_NPC_RECORDS];
	} file;
	int count = 0;
	const uint16_t *path = SAVE_FILE("npc.dat");

	for(int idx = 0; idx < world.numMarkers && count < MAX_NPC_RECORDS; idx++)
	{
		HouseMarker *marker = &world.markers[idx];
		file.records[count++] = (struct NpcRecord){
			.x = marker->position.x + stream.originX,
			.y = marker->position.y + stream.originY,
			.npc = (marker->occupant >= 0 && marker->occupant < world.numNPCs)
				? world.npcs[marker->occupant].id : NPC_NONE,
			.type = RECORD_MARKER
		};
	}
	for(int idx = 0; idx < world.numNPCs && count < MAX_NPC_RECORDS; idx++)
	{
		NPC *npc = &world.npcs[idx];
		if(npc->marker != -1) continue;
		file.records[count++] = (struct NpcRecord){
			.x = npc->props.x + stream.originX * 8,
			.y = npc->props.y + stream.originY * 8,
			.npc = npc->id,
			.type = RECORD_HOMELESS
		};
	}

	file.header = (struct NpcFileHeader){NPC_FILE_MAGIC, NPC_FILE_VERSION, count};
	return writeWholeFile(path, &file, sizeof(struct NpcFileHeader) + count * sizeof(struct NpcRecord));
}

void saveGame()
{
	struct SaveInfo info;
	struct PlayerSave playerSave;

	save.error = -99;
	save.timeTicks = world.timeTicks;

	if(!ensureSaveFolder())
	{
		save.error = -2;
		return;
	}

//	Chunks first: if anything goes wrong, the old save.info still describes a
//	consistent world
	if(!streamFlushOp())
	{
		save.error = -6;
		return;
	}

	if(!writeNpcFile())
	{
		save.error = -8;
		return;
	}

	memset(&playerSave, 0, sizeof(playerSave));
	playerSave.health = player.combat.health;
	playerSave.maxHealth = player.maxHealth;
	memcpy(playerSave.items, player.inventory.items, INVENTORY_SIZE * sizeof(Item));
	memcpy(playerSave.accessories, player.inventory.accessories, 5 * sizeof(Item));
	memcpy(playerSave.armour, player.inventory.armour, 3 * sizeof(Item));
	playerSave.spawnX = player.spawn.x;
	playerSave.spawnY = player.spawn.y;
	playerSave.x = player.props.x + stream.originX * 8;
	playerSave.y = player.props.y + stream.originY * 8;
	if(!writeWholeFile(SAVE_FILE("player.dat"), &playerSave, sizeof(playerSave)))
	{
		save.error = -1;
		return;
	}

	memset(&info, 0, sizeof(info));
	strncpy(info.version, VERSION, VERSION_BUFFER_SIZE - 1);
	info.time = save.timeTicks;
	info.worldSeed = save.worldSeed;
	if(!writeWholeFile(SAVE_FILE("save.info"), &info, sizeof(info))) save.error = -2;
}

static bool readWholeFile(const uint16_t *path, void *data, int size)
{
	int descriptor = BFile_Open(path, BFile_ReadOnly);

	if(descriptor < 0) return false;
	if(BFile_Size(descriptor) != size || BFile_Read(descriptor, data, size, 0) < 0)
	{
		BFile_Close(descriptor);
		return false;
	}
	BFile_Close(descriptor);
	return true;
}

/* Runs in the OS world. Returns the number of records read into `records`. */
static int readNpcFile(struct NpcRecord *records)
{
	struct NpcFileHeader header;
	int descriptor = BFile_Open(SAVE_FILE("npc.dat"), BFile_ReadOnly);
	int size;

	if(descriptor < 0) return 0;
	size = BFile_Size(descriptor);
	if(size < (int)sizeof(header) || BFile_Read(descriptor, &header, sizeof(header), 0) < 0
	|| header.magic != NPC_FILE_MAGIC || header.version != NPC_FILE_VERSION
	|| header.count > MAX_NPC_RECORDS
	|| size != (int)(sizeof(header) + header.count * sizeof(struct NpcRecord)))
	{
		BFile_Close(descriptor);
		return 0;
	}
	if(header.count > 0
	&& BFile_Read(descriptor, records, header.count * sizeof(struct NpcRecord), sizeof(header)) < 0)
	{
		BFile_Close(descriptor);
		return 0;
	}
	BFile_Close(descriptor);
	return header.count;
}

void loadSave()
{
	struct SaveInfo info;
	struct PlayerSave playerSave;
	struct PlayerSave *ps = &playerSave;

	save.error = -99;

	if(!readWholeFile(SAVE_FILE("save.info"), &info, sizeof(info)))
	{
		save.error = -2;
		return;
	}
	if(strncmp(info.version, VERSION, VERSION_BUFFER_SIZE) != 0)
	{
		save.error = -5;
		return;
	}
	if(!readWholeFile(SAVE_FILE("player.dat"), &playerSave, sizeof(playerSave)))
	{
		save.error = -1;
		return;
	}

	save.timeTicks = info.time;
	save.worldSeed = info.worldSeed;
	world.timeTicks = save.timeTicks;

	player.maxHealth = ps->maxHealth > 0 ? ps->maxHealth : 100;
	player.combat.health = ps->health;
	if(player.combat.health < 0) player.combat.health = 0;
	if(player.combat.health > player.maxHealth) player.combat.health = player.maxHealth;
	memcpy(player.inventory.items, ps->items, INVENTORY_SIZE * sizeof(Item));
	memcpy(player.inventory.accessories, ps->accessories, 5 * sizeof(Item));
	memcpy(player.inventory.armour, ps->armour, 3 * sizeof(Item));
	for(int slot = 0; slot < INVENTORY_SIZE; slot++) sanitizeItem(&player.inventory.items[slot]);
	for(int slot = 0; slot < 5; slot++) sanitizeItem(&player.inventory.accessories[slot]);
	for(int slot = 0; slot < 3; slot++) sanitizeItem(&player.inventory.armour[slot]);
	player.spawn.x = ps->spawnX;
	player.spawn.y = ps->spawnY;
	loadedX = ps->x;
	loadedY = ps->y;
}

void finishLoad()
{
	struct NpcRecord records[MAX_NPC_RECORDS];
	int numRecords;

	wgSetSeed(save.worldSeed);

	if(!streamInit(loadedX >> 3, loadedY >> 3)) memoryErrorMenu();
	player.props.x = loadedX - stream.originX * 8;
	player.props.y = loadedY - stream.originY * 8;

	numRecords = gint_world_switch(GINT_CALL(&readNpcFile, (void *)records));

//	Markers (and the NPCs living in them) first, then homeless NPCs. Housing
//	is re-validated later by the regular marker checks, since most markers
//	are outside the window right now.
	for(int pass = 0; pass < 2; pass++)
	{
		for(int i = 0; i < numRecords; i++)
		{
			struct NpcRecord *rec = &records[i];
			int numNPCs = world.numNPCs;
			NPC *npc;

			if(rec->npc < NPC_NONE || rec->npc > NPC_NURSE) rec->npc = NPC_NONE;
			if(pass == 0 && rec->type == RECORD_MARKER)
			{
				int numMarkers = world.numMarkers;
				Coords position = {rec->x - stream.originX, rec->y - stream.originY};

				addMarker(position);
				if(world.numMarkers == numMarkers || rec->npc == NPC_NONE) continue;
				addNPC(rec->npc);
				if(world.numNPCs == numNPCs) continue;
				npc = &world.npcs[world.numNPCs - 1];
				npc->props.x = position.x * 8;
				npc->props.y = position.y * 8;
				npc->marker = world.numMarkers - 1;
				world.markers[world.numMarkers - 1].occupant = world.numNPCs - 1;
			}
			else if(pass == 1 && rec->type == RECORD_HOMELESS && rec->npc != NPC_NONE)
			{
				addNPC(rec->npc);
				if(world.numNPCs == numNPCs) continue;
				npc = &world.npcs[world.numNPCs - 1];
				npc->props.x = rec->x - stream.originX * 8;
				npc->props.y = rec->y - stream.originY * 8;
			}
		}
	}
}
