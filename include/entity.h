#pragma once

/*
----- ENTITY -----

The entity handling system.
*/

#include <gint/display.h>

#include "inventory.h"
#include "menu.h"

enum PhysicsTypes {
	PHYS_NON_SOLID,
	PHYS_SOLID,
	PHYS_PLATFORM,
	PHYS_SAND
};

typedef struct {
	enum Items item;

	int amountMin;
	int amountMax;

	int ratioLow;
	int ratioHigh;
} Drop;

struct EntityDrops {
	int num;

	const Drop *dropList;
};

typedef struct {
	short x;
	short y;
} Coords;

struct Rect {
	Coords TL;
	Coords BR;
};

struct EntityPhysicsProps {
	int width;
	int height;
	int x;
	int y;
	float xVel;
	float yVel;
	bool touchingTileTop;
	bool dropping;
	bool movingSelf;
};

struct AnimationData {
	int animation;
	int animationFrame;
	int direction;
};

enum EntityAlignments {
	ALIGN_PEACEFUL,
	ALIGN_NEUTRAL,
	ALIGN_HOSTILE,
	ALIGN_SCARED
};

struct Combat {
	int health;

	enum EntityAlignments alignment;

	int immuneFrames;
	
	float attack;
	float defense;
	float knockbackResist;

	int currImmuneFrames;
};

enum Entities {
	ENT_SLIME,
	ENT_ZOMBIE,
	ENT_VULTURE,
	ENT_DEMONEYE,

	ENTITIES_COUNT
};

enum WaterPhysics {
	WATER_FRICTION,
	WATER_FLOAT
};

struct EntityBase {
	int id;
	struct EntityPhysicsProps props;
	struct Combat combat;
	bopti_image_t *sprite;
	const struct EntityDrops *drops;
	int spriteOffset;

	bool (*behaviour)(struct EntityBase *self, int frames);
	void (*init)(struct EntityBase *self);

//	Initialised to 0
	struct AnimationData anim;
	int despawnCounter;
//	To store states in, generic so that entities can use it however
	int mem[5];
};

typedef struct EntityBase Entity;

struct Player {
	struct EntityPhysicsProps props;
	struct AnimationData anim;
	struct Combat combat;

	Coords cursor;
	Coords cursorTile;
	Coords cursorWorld;
	struct GhostObject {
		short width;
		short height;
	} ghost;
	int useFrames;

	struct Inventory inventory;
	struct PlayerTool tool;
	struct AccessoryBonuses {
		int defense;
		bool doubleJump;
		bool hasDoubleJumped;
		float speedBonus;
	} bonuses;
	
	Coords spawn;

	int jumpTimer;
	bool jumpReleased;
	int swingFrame;

	int maxHealth;
	int ticksSinceHit;
	int pixelsFallen;
	int breath;
};

extern const struct EntityBase entityTemplates[];

extern struct Player player;

/* checkEntitySubmerged
Finds whether the entity is inside a water tile.

props: Pointer to the entity's physics properties
offsetY: Offset in pixels from the top of the entity to be used as the
submersion line.
*/
bool checkEntitySubmerged(struct EntityPhysicsProps *props, int offsetY);


/* handlePhysics
Performs a physics update on the given EntityPhysicsProps struct.

self: Pointer to the EntityPhysicsProps struct.
frames: Frames passed
onlyCollisions: Whether to only resolve tile collisions.
water: What to do if the entity is touching water.
*/
void handlePhysics(struct EntityPhysicsProps *self, int frames, bool onlyCollisions, enum WaterPhysics water);

/* checkCollision
Checks two EntityPhysicsProps structs for a collision between the two.

first: Pointer to the first EntityPhycisProps struct.
second: Pointer to the second EntityPhysicsProps struct.

Returns true if a collision has occured, otherwise false.
*/
bool checkCollision(struct EntityPhysicsProps *first, struct EntityPhysicsProps *second);

/* attack
Performs a player attack on an entity or an entity attack on the player.
Handles damage, knockback, and immunity frames.

entity: Pointer to the involved entity.
isPlayerAttacking: Indicates whether the player is the one performing the
attack, or the entity.
*/
void attack(Entity *entity, bool isPlayerAttacking);

/* doEntityCycle
Handles behaviour, attacks, and drops, for all entities.

frames: The.amount of frames passed since the world was loaded.
*/
void doEntityCycle(int frames);

/* doSpawningCycle
Handles spawning and despawning of entities.
*/
void doSpawningCycle();
