#pragma once

/*
----- RENDER -----

The rendering system, particle explosion engine, and utility functions.
*/

#include "inventory.h"

enum SpriteTypes {
	TYPE_TILE,
	TYPE_TILE_VAR,
	TYPE_SHEET,
	TYPE_SHEET_VAR
};

typedef struct {
	int x;
	int y;
	float xVel;
	float yVel;
} Particle;

struct ParticleExplosion {
	int numParticles;
	Particle *particles;
	int deltaTicks;
};

typedef struct {
	unsigned char pixel0 : 4;
	unsigned char pixel1 : 4;
} Pair;

/* render
Renders the world using a calculated camera position and a HUD.
*/
void render();

/* takeVRAMCapture
Stores the state of the drawing VRAMS to capt.vram.
*/
void takeVRAMCapture();

/* renderItem
Renders the given item.

x, y: Screen-relative coordinates to render the item at.
item: Pointer to the item to render.
*/
void renderItem(int x, int y, Item *item);

/* createExplosion
Initialises a random explosion at the given coordinates.

explosion: Pointer to an explosion struct. NOTE: The particles variable *MUST*
have been allocated to `50 * sizeof Particle` bytes!
x, y: Pixel coordinates of the explosion's origin.
*/
void createExplosion(struct ParticleExplosion *explosion, int x, int y);

/* destroyExplosion
Destructor for the given explosion. Frees the particle buffer.

explosion: Pointer to the explosion to destruct.
*/
void destroyExplosion(struct ParticleExplosion *explosion);

/* renderAndUpdateExplosion
Renders and updates the given explosion by one frame.

explosion: Pointer to the explosion.
offsetX, offsetY: Delta coordinates to be used with a camera.
*/
void renderAndUpdateExplosion(struct ParticleExplosion *explosion, int offsetX, int offsetY);

/* middleText
Clears the screen and displays some text in the middle.
Renders a progress bar.
Meant to be used with world generation.

text: Pointer to the string to display.
progress: Percent progress.
*/
void middleText(char *text, int progress);