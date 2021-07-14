#pragma once

/*
----- DEFINITIONS -----

Various macros, constant definitions, and utility functions used throughout the game.
*/

/*
Disables certain game features to make testing easier. Adds warnings in the
main menu and game version in case it's accidentally left enabled.
*/
//#define DEBUGMODE

enum HWModes {
	MODE_RAM,
	MODE_PRAM
};

struct GameCompatibilityPresets {
	enum HWModes HWMODE;
	void *RAM_START;
	int WORLD_WIDTH;
	int WORLD_HEIGHT;
	float WORLDGEN_MULTIPLIER;
};

extern struct GameCompatibilityPresets game;

#define allocCheck(x) if((x) == NULL) memoryErrorMenu()

#ifndef DEBUGMODE
#define VERSION "v0.9.1-indev"
#else
#define VERSION "DEBUG BUILD!"
#endif

#define PI 3.14159265358979323846
#define E 2.718281828459045

// Size in tiles of each world region
#define REGION_SIZE 90

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define VAR_BUF_WIDTH 28
#define VAR_BUF_HEIGHT 20
#define VAR_BUF_OFFSET 5

#define MAX_CHESTS 50

#define CHECK_BUFFER_SIZE 750

#define GRAVITY_ACCEL 0.15

#define INVENTORY_SIZE 24

// Arbitrary size, not using GYRAM for much else
#define MAX_REGIONS 1000

// 12 minutes
#define DAY_TICKS 43200

// Amount of entity slots
#define MAX_ENTITIES 12

// 1/<x> chance to spawn an entity each frame
#define SPAWN_CHANCE 800

/*
Must be even.
This WILL break any old saves if changed! Make sure you have a very good reason
before doing so!
*/
#define VERSION_BUFFER_SIZE 16

#define MARKER_CHECK_DISTANCE 30

// Callback used to govern loops that have a framerate
int frameCallback(volatile int *flag);
