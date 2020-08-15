#pragma once

enum SpriteTypes {
	TYPE_TILE,
	TYPE_TILE_VAR,
	TYPE_SHEET,
	TYPE_SHEET_VAR
};

void render();
void takeVRAMCapture();
void renderItem();