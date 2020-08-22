#pragma once

enum SpriteTypes {
	TYPE_TILE,
	TYPE_TILE_VAR,
	TYPE_SHEET,
	TYPE_SHEET_VAR
};

typedef struct {
	unsigned char pixel0 : 4;
	unsigned char pixel1 : 4;
} Pair;

void render();
void takeVRAMCapture();
void renderItem();