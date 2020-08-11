#pragma once

enum SpriteTypes {
	TYPE_TILE,
	TYPE_TILE_VAR,
	TYPE_SHEET,
	TYPE_SHEET_VAR
};

int render();
void takeVRAMCapture();
void renderItem();