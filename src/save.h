#pragma once

#include "entity.h"

#define REGIONS_X (WORLD_WIDTH / REGION_SIZE + 1)
#define REGIONS_Y (WORLD_HEIGHT / REGION_SIZE + 1)
#define NUM_REGIONS REGIONS_X * REGIONS_Y

struct SaveData {
	int tileDataSize;
	int regionsX;
	int regionsY;
	unsigned char *tileData;
	char regionData[NUM_REGIONS];
	int error;
};

struct PlayerSave {
	Item items[INVENTORY_SIZE];
	int health;
};

extern struct SaveData save;
extern struct Player player;

void saveGame();
void loadSave();
bool getSave();