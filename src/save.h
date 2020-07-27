#pragma once

#include "entity.h"

struct SaveData {
	int tileDataSize;
	int regionsX;
	int regionsY;
	unsigned char* tileData;
	unsigned char* regionData;
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