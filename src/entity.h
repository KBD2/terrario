#pragma once

struct Player {
	int x;
	int y;
	unsigned char width;
	unsigned char height;

	void (*update)(struct Player* self);
};

void updatePlayer(struct Player* self);