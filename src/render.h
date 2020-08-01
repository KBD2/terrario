#pragma once

enum SpriteTypes {
	TILE,
	TILE_VAR,
	SHEET,
	SHEET_VAR
};

int render();
void takeVRAMCapture();
void renderItem();