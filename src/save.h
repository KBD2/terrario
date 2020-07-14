#pragma once

struct SaveData {
	int tileDataSize;
	int regionsX;
	int regionsY;
	unsigned char* tileData;
	unsigned char* regionData;
	int error;
};

extern struct SaveData save;

void saveGame();
void loadSave();
bool getSave();