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
	color_t colour;
} Particle;

struct ParticleExplosion {
	int numParticles;
	Particle *particles;
	int deltaTicks;
};

extern char varBuffer[VAR_BUF_HEIGHT][VAR_BUF_WIDTH];

/* render
Renders the world using a calculated camera position and a HUD.

renderHUD: Whether to render the HUD.
*/
void render(bool renderHUD);

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
Updates the given explosion by one frame.

explosion: Pointer to the explosion.
offsetX, offsetY: Delta coordinates to be used with a camera.
*/
void updateExplosion(struct ParticleExplosion *explosion);

/* middleText
Clears the screen and displays some text in the middle.
Renders a progress bar.
Meant to be used with world generation.

text: Pointer to the string to display.
progress: Percent progress.
*/
void middleText(char *text, int progress);

/* fillVarBuffer
Fills the variant buffer's given subrectangle.

startX, startY: Coordinates of the top left point of the rectangle to fill.
width, height: Sizes of the rectangle to fill (must not exceed buffer boundaries).
*/
void fillVarBuffer(int startX, int startY, int width, int height);

/* updateVarBuffer
Shifts around and fills in the variant buffer.

x, y: Tile coordinates to use as deltas.
*/
void updateVarBuffer(int x, int y);

/* setVar
Either randomises the variant or computes the variant of the given tile.
Does nothing if the tile is out of the variant buffer window.

x, y: World coordinates of the tile.
*/
void setVar(int x, int y);