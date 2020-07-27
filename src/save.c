#include <gint/bfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "save.h"
#include "defs.h"
#include "world.h"

// Checks if \\fls0\TERRARIO\reg0.dat exists
bool getSave()
{
	int handle;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	error = BFile_FindFirst((const uint16_t*)u"\\\\fls0\\TERRARIO\\reg0.dat", &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);

	return error == 0 ? true : false;
}

void saveGame()
{
	const uint16_t* folderPath = u"\\\\fls0\\TERRARIO";
	const uint16_t* playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	int handle;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	Tile* tile;
	int regionStartX, regionStartY;

	char buffer[30];
	uint16_t filePath[30];
	Tile regionBuffer[REGION_SIZE * REGION_SIZE];
	int descriptor;
	int size = sizeof(regionBuffer);

	struct PlayerSave playerSave;
	playerSave.health = player.health;
	memcpy(playerSave.items, player.inventory.items, INVENTORY_SIZE * sizeof(Item));

	int playerSaveSize = sizeof(struct PlayerSave);

	error = BFile_FindFirst(folderPath, &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);
	if(error == -1) BFile_Create(folderPath, BFile_Folder, NULL);

	BFile_Remove(playerPath);
	BFile_Create(playerPath, BFile_File, &playerSaveSize);
	descriptor = BFile_Open(playerPath, BFile_WriteOnly);
	BFile_Write(descriptor, (void*)&playerSave, playerSaveSize);
	BFile_Close(descriptor);

	for(int y = 0; y < save.regionsY; y++)
	{
		for(int x = 0; x < save.regionsX; x++)
		{
			if(save.regionData[y * save.regionsX + x])
			{
				regionStartX = x * REGION_SIZE;
				regionStartY = y * REGION_SIZE;
				for(int tileY = regionStartY; tileY < regionStartY + REGION_SIZE; tileY++)
				{
					for(int tileX = regionStartX; tileX < regionStartX + REGION_SIZE; tileX++)
					{
						tile = &regionBuffer[(tileY - regionStartY) * REGION_SIZE + (tileX - regionStartX)];
						if(tileX >= WORLD_WIDTH || tileY >= WORLD_HEIGHT)
						{
							*tile = (Tile){ 0 };
						}
						else
						{
							*tile = world.tiles[tileY * WORLD_WIDTH + tileX];
						}
					}
				}

				sprintf(buffer, "\\\\fls0\\TERRARIO\\reg%d.dat", (y << 4) | x);
				for(int i = 0; i < 30; i++) filePath[i] = buffer[i];
				BFile_Remove(filePath);
				error = BFile_Create(filePath, BFile_File, &size);
				if(error < 0)
				{
					save.error = (y << 4) | x;
					return;
				}

				descriptor = BFile_Open(filePath, BFile_WriteOnly);
				BFile_Write(descriptor, regionBuffer, size);
				BFile_Close(descriptor);
			}
		}
		
	}
}

void loadSave()
{
	char* regionFilePath = "\\\\fls0\\TERRARIO\\reg%d.dat";
	const uint16_t* playerPath = u"\\\\fls0\\TERRARIO\\player.dat";
	char buffer[30];
	uint16_t filePath[30];

	int handle;
	uint16_t foundPath[30];
	struct BFile_FileInfo fileInfo;
	int error;

	int descriptor;
	Tile regionBuffer[REGION_SIZE * REGION_SIZE];

	int regionStartX, regionStartY;

	struct PlayerSave playerSave;

	error = BFile_FindFirst(playerPath, &handle, foundPath, &fileInfo);
	BFile_FindClose(handle);
	if(error < 0)
	{
		save.error = -1;
		return;
	}
	descriptor = BFile_Open(playerPath, BFile_ReadOnly);
	BFile_Read(descriptor, (void*)&playerSave, sizeof(struct PlayerSave), 0);
	BFile_Close(descriptor);

	player.health = playerSave.health;
	memcpy(player.inventory.items, playerSave.items, INVENTORY_SIZE * sizeof(Item));

	for(int y = 0; y < save.regionsY; y++)
	{
		for(int x = 0; x < save.regionsX; x++)
		{
			sprintf(buffer, regionFilePath, (y << 4) | x);
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
					if(tileX >= WORLD_WIDTH || tileY >= WORLD_HEIGHT) continue;
					world.tiles[tileY * WORLD_WIDTH + tileX] = regionBuffer[(tileY - regionStartY) * REGION_SIZE + (tileX - regionStartX)];
				}
			}
		}
		
	}
}