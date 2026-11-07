#pragma once

/*
----- STREAM -----

Keeps a small window of chunks around the player in RAM and slides it over
the infinite world as the player moves.

The window is WINDOW_CHUNKS_X x WINDOW_CHUNKS_Y chunks of CHUNK_SIZE tiles
(128 x 96 tiles, 12 kB). It is stored as one row-major array (world.tiles), so
the rest of the game keeps using plain window-local coordinates (0..127,
0..95). stream.originX/originY give the global tile coordinates of the
window's top-left corner.

When the player gets close to an edge the window moves by one chunk: chunks
that leave are written to storage if the player changed them, the remaining
ones are shifted in memory, new chunks are loaded from storage or generated,
and everything holding window-local coordinates (player, entities, NPCs,
markers...) is shifted to match.
*/

#include <stdbool.h>
#include <stdint.h>

#include "worldgen.h"

#define WINDOW_CHUNKS_X 4
#define WINDOW_CHUNKS_Y 3
#define WINDOW_WIDTH (WINDOW_CHUNKS_X * CHUNK_SIZE)
#define WINDOW_HEIGHT (WINDOW_CHUNKS_Y * CHUNK_SIZE)

/* Distance (in tiles) from the window edge at which the window moves */
#define SHIFT_MARGIN_X 32
#define SHIFT_MARGIN_Y 16

/* World border, in tiles. Keeps file names short (see save.h). */
#define WORLD_LIMIT_X 524288
#define WORLD_LIMIT_Y 16384

struct Stream {
	int originX;
	int originY;
	unsigned char dirty[WINDOW_CHUNKS_Y][WINDOW_CHUNKS_X];
	uint8_t *ioBuffer;
	int error;
};

extern struct Stream stream;

static inline void streamMarkDirty(int x, int y)
{
	if((unsigned)x >= WINDOW_WIDTH || (unsigned)y >= WINDOW_HEIGHT) return;
	stream.dirty[y >> CHUNK_SHIFT][x >> CHUNK_SHIFT] = 1;
}

static inline bool streamIsDirty(int x, int y)
{
	if((unsigned)x >= WINDOW_WIDTH || (unsigned)y >= WINDOW_HEIGHT) return false;
	return stream.dirty[y >> CHUNK_SHIFT][x >> CHUNK_SHIFT];
}

/* streamInit
Fills the whole window around the given global tile, loading saved chunks and
generating the others. Shows a progress bar.

Returns false if memory could not be allocated.
*/
bool streamInit(int globalTileX, int globalTileY);

/* streamUpdate
Called every frame. Moves the window if the player is near one of its edges.

Returns false if modified chunks could not be saved (stream.error is set).
*/
bool streamUpdate();

/* streamTeleport
Moves the player to the given global pixel position, moving the window as
needed (used for respawning and the magic mirror).

Returns false if modified chunks could not be saved.
*/
bool streamTeleport(int globalPixelX, int globalPixelY);

/* streamFlushOp
Writes every modified chunk of the window to storage. Must be called inside a
gint_world_switch().

Returns true on success.
*/
bool streamFlushOp();

/* streamFree
Releases the I/O buffer.
*/
void streamFree();
