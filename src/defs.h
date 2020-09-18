#pragma once

/*
----- DEFINITIONS -----

Various macros, constant definitions, and utility functions used throughout the game.
*/

// Only use for fx9750GIII builds
//#define USE_HEAP

#define allocCheck(x) if((x) == NULL) memoryErrorMenu()

#define VERSION "v0.5.1-indev"

// Start of the 250kB static RAM region used to store the world
#define RAM_START 0x88040000
// Size in tiles of each world region
#define REGION_SIZE 96

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#ifndef USE_HEAP
#define WORLD_WIDTH 1000
#define WORLD_HEIGHT 250
#else
#define WORLD_WIDTH 400
#define WORLD_HEIGHT 100
#endif

#define GRAVITY_ACCEL 0.15

#define INVENTORY_SIZE 24

//.amount of entity slots
#define MAX_ENTITIES 5
// 1/<x> chance to spawn an entity each frame
#define SPAWN_CHANCE 600

// Callback used to govern loops that have a framerate
int frameCallback(volatile int *flag);