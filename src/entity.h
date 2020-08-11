#pragma once

#include "inventory.h"
#include "menu.h"

enum PhysicsTypes {
	PHYS_NON_SOLID,
	PHYS_SOLID,
	PHYS_PLATFORM
};

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
	bool dropping;
};

struct AnimationData {
	int animation;
	int animationFrame;
	int direction;
};

struct Player {
	struct EntPhysicsProps props;
	int health;
	struct Coords cursor;
	struct Coords cursorTile;
	struct AnimationData anim;
	struct Inventory inventory;

	void (*physics)(struct EntPhysicsProps* self);
};

extern struct Player player;

void handlePhysics(struct EntPhysicsProps* self);