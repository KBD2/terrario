#pragma once

#include "map.h"

struct Coords {
	int x;
	int y;
};

struct Rect {
	struct Coords TL;
	struct Coords BR;
};

struct EntPhysicsProps {
	int x;
	int y;
	float xVel;
	float yVel;
	bool touchingTileTop;
	unsigned char width;
	unsigned char height;
};

struct Player {
	struct EntPhysicsProps props;
	int health;
	struct Coords cursor;
	struct Coords cursorTile;

	void (*update)(struct Map* map, struct Player* self);
	void (*collisions)(struct Map* map, struct EntPhysicsProps* self);
};

void updatePlayer(struct Map* map, struct Player* self);
void handleCollisions(struct Map* map, struct EntPhysicsProps* self);