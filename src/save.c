#include <gint/bfile.h>
#include <stdbool.h>
#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <gint/std/stdlib.h>

#include "save.h"
#include "defs.h"
#include "world.h"

struct SaveInfo {
	char version[VERSION_BUFFER_SIZE];
	int time;
};

// Checks if \\fls0\TERRARIO\save.info exists
bool getSave()
{
	int handle;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst((const uint16_t*)u"\\\\fls0\\TERRARIO\\save.info", &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);

	return error == 0 ? true : false;
}

void getVersionInfo()
{
	extern char versionBuffer[16];
	struct SaveInfo info;
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	int descriptor = BFile_Open(infoPath, BFile_ReadOnly);

	BFile_Read(descriptor, &info, sizeof(struct SaveInfo), 0);
	BFile_Close(descriptor);
	strncpy(versionBuffer, info.version, VERSION_BUFFER_SIZE);
}

void dumpRegions()
{
	const uint16_t *regionPath = u"\\\\fls0\\regions.dat";
	int size = (save.regionsX * save.regionsY + 1) & ~1;
	BFile_Remove(regionPath);
	BFile_Create(regionPath, BFile_File, &size);
	int d = BFile_Open(regionPath, BFile_WriteOnly);
	BFile_Write(d, (void*)save.regionData, size);
	BFile_Close(d);
}

void saveGame()
{
	//dumpRegions();
	const uint16_t *folderPath = u"\\\\fls0\\TERRARIO";
	const uint16_t *playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	const uint16_t *chestPath = u"\\\\fls0\\TERRARIO\\chests.dat";

	int handle;
	int descriptor;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	Tile *tile;
	int regionStartX, regionStartY;
	Tile regionBuffer[REGION_SIZE * REGION_SIZE];

	char buffer[30];
	uint16_t filePath[30];

	struct SaveInfo info;

	int regionFileSize = sizeof(regionBuffer);
	int playerSaveSize = sizeof(struct PlayerSave);
	int infoFileSize = sizeof(struct SaveInfo);
	int chestDataSize = world.chests.number * sizeof(struct Chest);

	save.timeTicks = world.timeTicks;

//	Create the folder if it doesn't exist
	error = BFile_FindFirst(folderPath, &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);
	if(error == -1) BFile_Create(folderPath, BFile_Folder, NULL);

//	Create save.info and put the version and day ticks inside
	strncpy(info.version, VERSION, VERSION_BUFFER_SIZE);
	info.time = save.timeTicks;
	BFile_Remove(infoPath);
	BFile_Create(infoPath, BFile_File, &infoFileSize);
	descriptor = BFile_Open(infoPath, BFile_WriteOnly);
	BFile_Write(descriptor, &info, sizeof(info));
	BFile_Close(descriptor);

//	Create player.dat and put in player information and inventory
	struct PlayerSave playerSave;
	playerSave.health = player.combat.health;
	memcpy(playerSave.items, player.inventory.items, INVENTORY_SIZE * sizeof(Item));
	BFile_Remove(playerPath);
	BFile_Create(playerPath, BFile_File, &playerSaveSize);
	descriptor = BFile_Open(playerPath, BFile_WriteOnly);
	BFile_Write(descriptor, &playerSave, playerSaveSize);
	BFile_Close(descriptor);

//	Create chests.dat and put in all the world chests
	BFile_Remove(chestPath);
	if(chestDataSize > 0)
	{
		BFile_Create(chestPath, BFile_File, &chestDataSize);
		descriptor = BFile_Open(chestPath, BFile_WriteOnly);
		BFile_Write(descriptor, world.chests.chests, chestDataSize);
		BFile_Close(descriptor);
	}

	for(int y = 0; y < save.regionsY; y++)
	{
		for(int x = 0; x < save.regionsX; x++)
		{
			if(save.regionData[y * save.regionsX + x] == 1)
			{
				regionStartX = x * REGION_SIZE;
				regionStartY = y * REGION_SIZE;
				for(int tileY = regionStartY; tileY < regionStartY + REGION_SIZE; tileY++)
				{
					for(int tileX = regionStartX; tileX < regionStartX + REGION_SIZE; tileX++)
					{
						tile = &regionBuffer[(tileY - regionStartY) * REGION_SIZE + (tileX - regionStartX)];
						if(tileX >= game.WORLD_WIDTH || tileY >= game.WORLD_HEIGHT)
						{
							*tile = (Tile){ 0 };
						}
						else
						{
							*tile = getTile(tileX, tileY);
						}
					}
				}

				sprintf(buffer, "\\\\fls0\\TERRARIO\\reg%i-%i.dat", y, x);
				for(int i = 0; i < 30; i++) filePath[i] = buffer[i];
				BFile_Remove(filePath);
				error = BFile_Create(filePath, BFile_File, &regionFileSize);
				if(error < 0)
				{
					save.error = (y << 4) | x;
					return;
				}

				descriptor = BFile_Open(filePath, BFile_WriteOnly);
				BFile_Write(descriptor, regionBuffer, regionFileSize);
				BFile_Close(descriptor);
			}
		}
	}
}

void loadSave()
{
	char *regionFilePath = "\\\\fls0\\TERRARIO\\reg%i-%i.dat";
	const uint16_t *playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	const uint16_t *infoPath = u"\\\\fls0\\TERRARIO\\save.info";
	const uint16_t *chestPath = u"\\\\fls0\\TERRARIO\\chests.dat";
	char buffer[30];
	uint16_t filePath[30];

	struct SaveInfo info;

	int handle;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	int descriptor;
	Tile regionBuffer[REGION_SIZE * REGION_SIZE];

	int regionStartX, regionStartY;

	struct PlayerSave playerSave;

	Tile tile;

	error = BFile_FindFirst(playerPath, &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);
	if(error < 0)
	{
		save.error = -1;
		return;
	}
	descriptor = BFile_Open(playerPath, BFile_ReadOnly);
	BFile_Read(descriptor, (void *)&playerSave, sizeof(struct PlayerSave), 0);
	BFile_Close(descriptor);

	descriptor = BFile_Open(infoPath, BFile_ReadOnly);
	BFile_Read(descriptor, &info, sizeof(struct SaveInfo), 0);
	BFile_Close(descriptor);

	save.timeTicks = info.time;
	world.timeTicks = save.timeTicks;

	error = BFile_FindFirst(chestPath, &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);
	if(error == 0)
	{
		descriptor = BFile_Open(chestPath, BFile_ReadOnly);
		world.chests.number = fileInfo.data_size / sizeof(struct Chest);
		BFile_Read(descriptor, (void *)world.chests.chests, fileInfo.data_size, 0);
		BFile_Close(descriptor);
	}

	player.combat.health = playerSave.health;
	memcpy(player.inventory.items, playerSave.items, INVENTORY_SIZE * sizeof(Item));

	for(int y = 0; y < save.regionsY; y++)
	{
		for(int x = 0; x < save.regionsX; x++)
		{
			sprintf(buffer, regionFilePath, y, x);
			for(int i = 0; i < 30; i++) filePath[i] = buffer[i];
			error = BFile_FindFirst(filePath, &handle, foundPath, &fileInfo);
			BFile_FindClose(handle);
			if(error < 0)
			{
				save.error = (y << 4) | x;
				return;
			}

			descriptor = BFile_Open(filePath, BFile_ReadOnly);
			BFile_Read(descriptor, regionBuffer, sizeof(regionBuffer), 0);
			BFile_Close(descriptor);

			regionStartX = x * REGION_SIZE;
			regionStartY = y * REGION_SIZE;
			for(int tileY = regionStartY; tileY < regionStartY + REGION_SIZE; tileY++)
			{
				for(int tileX = regionStartX; tileX < regionStartX + REGION_SIZE; tileX++)
				{
					if(tileX >= game.WORLD_WIDTH || tileY >= game.WORLD_HEIGHT) continue;
					tile = regionBuffer[(tileY - regionStartY) * REGION_SIZE + (tileX - regionStartX)];
					setTile(tileX, tileY, tile.id);
				}
			}
		}
	}
}