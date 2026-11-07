#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gint/bfile.h>
#include <gint/gint.h>

#include "stream.h"
#include "worldgen.h"
#include "world.h"
#include "save.h"
#include "render.h"
#include "npc.h"
#include "defs.h"

struct Stream stream;

#define CHUNK_MAGIC 0x4B4E4843u
#define CHUNK_VERSION 3
#define CHUNK_BYTES (CHUNK_SIZE * CHUNK_SIZE)
#define NUM_SLOTS (WINDOW_CHUNKS_X * WINDOW_CHUNKS_Y)

struct ChunkFileHeader {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	int32_t chunkX;
	int32_t chunkY;
	uint32_t seed;
};

_Static_assert(sizeof(struct ChunkFileHeader) % 2 == 0, "chunk header must be even-sized");
_Static_assert((CHUNK_BYTES) % 2 == 0, "chunk data must be even-sized");

/* Pending chunk I/O, filled in before a gint_world_switch() */
static int ioCount;
static int ioChunkX[NUM_SLOTS], ioChunkY[NUM_SLOTS];
static int ioSlotX[NUM_SLOTS], ioSlotY[NUM_SLOTS];
static bool ioLoaded[NUM_SLOTS];

static inline int floorDivInt(int a, int b)
{
	return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static inline uint8_t *slotPtr(int sx, int sy)
{
	return (uint8_t *)world.tiles + (sy * CHUNK_SIZE) * WINDOW_WIDTH + sx * CHUNK_SIZE;
}

/* Chunk files are named C + 3 base-32 digits of X + 2 digits of Y */
static void chunkPath(int chunkX, int chunkY, uint16_t *path)
{
	char name[16];

	name[0] = 'C';
	encodeName(name + 1, (unsigned)(chunkX + (WORLD_LIMIT_X >> CHUNK_SHIFT)), 3);
	encodeName(name + 4, (unsigned)(chunkY + (WORLD_LIMIT_Y >> CHUNK_SHIFT)), 2);
	strcpy(name + 6, ".dat");
	savePath(name, path);
}

static bool writeChunkFile(int chunkX, int chunkY)
{
	uint16_t path[48];
	struct ChunkFileHeader header = {
		.magic = CHUNK_MAGIC,
		.version = CHUNK_VERSION,
		.size = CHUNK_SIZE,
		.chunkX = chunkX,
		.chunkY = chunkY,
		.seed = save.worldSeed
	};
	int size = sizeof(header) + CHUNK_BYTES;
	int descriptor;

	chunkPath(chunkX, chunkY, path);
	BFile_Remove(path);
	if(BFile_Create(path, BFile_File, &size) < 0) return false;
	descriptor = BFile_Open(path, BFile_WriteOnly);
	if(descriptor < 0) return false;
	if(BFile_Write(descriptor, &header, sizeof(header)) < 0
	|| BFile_Write(descriptor, stream.ioBuffer, CHUNK_BYTES) < 0)
	{
		BFile_Close(descriptor);
		return false;
	}
	BFile_Close(descriptor);
	return true;
}

/* Reads a chunk file into the I/O buffer. Missing files, files from another
   world and damaged files all count as "not saved": the chunk is then simply
   generated again. */
static bool readChunkFile(int chunkX, int chunkY)
{
	uint16_t path[48];
	struct ChunkFileHeader header;
	int descriptor;

	chunkPath(chunkX, chunkY, path);
	descriptor = BFile_Open(path, BFile_ReadOnly);
	if(descriptor < 0) return false;
	if(BFile_Size(descriptor) != (int)(sizeof(header) + CHUNK_BYTES)
	|| BFile_Read(descriptor, &header, sizeof(header), 0) < 0)
	{
		BFile_Close(descriptor);
		return false;
	}
	if(header.magic != CHUNK_MAGIC || header.version != CHUNK_VERSION
	|| header.size != CHUNK_SIZE || header.chunkX != chunkX
	|| header.chunkY != chunkY || header.seed != save.worldSeed)
	{
		BFile_Close(descriptor);
		return false;
	}
	if(BFile_Read(descriptor, stream.ioBuffer, CHUNK_BYTES, sizeof(header)) < 0)
	{
		BFile_Close(descriptor);
		return false;
	}
	BFile_Close(descriptor);

	for(int i = 0; i < CHUNK_BYTES; i++)
	{
		if(stream.ioBuffer[i] >= TILES_COUNT) stream.ioBuffer[i] = TILE_STONE;
	}
	return true;
}

static void copyOut(int sx, int sy)
{
	uint8_t *src = slotPtr(sx, sy);
	for(int row = 0; row < CHUNK_SIZE; row++)
		memcpy(stream.ioBuffer + row * CHUNK_SIZE, src + row * WINDOW_WIDTH, CHUNK_SIZE);
}

static void copyIn(int sx, int sy)
{
	uint8_t *dest = slotPtr(sx, sy);
	for(int row = 0; row < CHUNK_SIZE; row++)
		memcpy(dest + row * WINDOW_WIDTH, stream.ioBuffer + row * CHUNK_SIZE, CHUNK_SIZE);
}

/* Runs in the OS world. Returns the number of chunks that failed to save. */
static int saveChunksOp()
{
	int failures = 0;

	if(!ensureSaveFolder()) return ioCount;
	for(int k = 0; k < ioCount; k++)
	{
		copyOut(ioSlotX[k], ioSlotY[k]);
		if(!writeChunkFile(ioChunkX[k], ioChunkY[k])) failures++;
	}
	return failures;
}

/* Runs in the OS world */
static int loadChunksOp()
{
	for(int k = 0; k < ioCount; k++)
	{
		ioLoaded[k] = readChunkFile(ioChunkX[k], ioChunkY[k]);
		if(ioLoaded[k]) copyIn(ioSlotX[k], ioSlotY[k]);
	}
	return 0;
}

bool streamFlushOp()
{
	ioCount = 0;
	for(int sy = 0; sy < WINDOW_CHUNKS_Y; sy++)
	{
		for(int sx = 0; sx < WINDOW_CHUNKS_X; sx++)
		{
			if(!stream.dirty[sy][sx]) continue;
			ioChunkX[ioCount] = (stream.originX >> CHUNK_SHIFT) + sx;
			ioChunkY[ioCount] = (stream.originY >> CHUNK_SHIFT) + sy;
			ioSlotX[ioCount] = sx;
			ioSlotY[ioCount] = sy;
			ioCount++;
		}
	}
	if(ioCount == 0) return true;
	if(saveChunksOp() != 0) return false;
	memset(stream.dirty, 0, sizeof(stream.dirty));
	return true;
}

/* Everything that stores window-local coordinates moves with the window */
static void shiftObjects(int dTilesX, int dTilesY)
{
	int dx = dTilesX * 8, dy = dTilesY * 8;
	int maxX = WINDOW_WIDTH * 8, maxY = WINDOW_HEIGHT * 8;

	player.props.x += dx;
	player.props.y += dy;
	player.cursorWorld.x += dx;
	player.cursorWorld.y += dy;
	player.cursorTile.x += dTilesX;
	player.cursorTile.y += dTilesY;
	if(player.tool.type == TOOL_TYPE_PICK)
	{
		player.tool.data.pickData.targeted.x += dTilesX;
		player.tool.data.pickData.targeted.y += dTilesY;
	}

	for(int idx = 0; idx < MAX_ENTITIES; idx++)
	{
		Entity *ent = &world.entities[idx];
		if(ent->id == -1) continue;
		ent->props.x += dx;
		ent->props.y += dy;
//		Monsters that end up outside the window are despawned
		if(ent->props.x < 0 || ent->props.y < 0
		|| ent->props.x + ent->props.width >= maxX
		|| ent->props.y + ent->props.height >= maxY) world.removeEntity(idx);
	}

//	NPCs outside the window are kept (frozen) until the player comes back
	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		world.npcs[idx].props.x += dx;
		world.npcs[idx].props.y += dy;
	}
	for(int idx = 0; idx < world.numMarkers; idx++)
	{
		world.markers[idx].position.x += dTilesX;
		world.markers[idx].position.y += dTilesY;
	}

	for(int i = 0; i < world.explosion.numParticles; i++)
	{
		world.explosion.particles[i].x += dx;
		world.explosion.particles[i].y += dy;
	}

//	The variant buffer still describes the same tiles
	varBufferPos.x += dTilesX;
	varBufferPos.y += dTilesY;
}

static int clampOrigin(int value, int limit, int size)
{
	if(value < -limit) return -limit;
	if(value > limit - size) return limit - size;
	return value;
}

/* Moves the window so that its top-left corner is at global tile
   (newX, newY). With haveOld false, the window is filled from scratch. */
static bool moveWindow(int newX, int newY, bool haveOld, bool showProgress)
{
	int dcx = haveOld ? (newX - stream.originX) / CHUNK_SIZE : 0;
	int dcy = haveOld ? (newY - stream.originY) / CHUNK_SIZE : 0;
	bool overlap = haveOld && abs(dcx) < WINDOW_CHUNKS_X && abs(dcy) < WINDOW_CHUNKS_Y;
	bool retained[WINDOW_CHUNKS_Y][WINDOW_CHUNKS_X];
	unsigned char dirty[WINDOW_CHUNKS_Y][WINDOW_CHUNKS_X];
	int oldX = stream.originX, oldY = stream.originY;
	int done = 0, total;

//	1. Save modified chunks that are leaving the window
	if(haveOld)
	{
		ioCount = 0;
		for(int sy = 0; sy < WINDOW_CHUNKS_Y; sy++)
		{
			for(int sx = 0; sx < WINDOW_CHUNKS_X; sx++)
			{
				int nsx = sx - dcx, nsy = sy - dcy;
				bool stays = overlap && nsx >= 0 && nsx < WINDOW_CHUNKS_X && nsy >= 0 && nsy < WINDOW_CHUNKS_Y;
				if(stays || !stream.dirty[sy][sx]) continue;
				ioChunkX[ioCount] = (oldX >> CHUNK_SHIFT) + sx;
				ioChunkY[ioCount] = (oldY >> CHUNK_SHIFT) + sy;
				ioSlotX[ioCount] = sx;
				ioSlotY[ioCount] = sy;
				ioCount++;
			}
		}
		if(ioCount > 0 && gint_world_switch(GINT_CALL(saveChunksOp)) != 0)
		{
			stream.error = -6;
			return false;
		}
	}

//	2. Shift the chunks that stay. The window is one row-major array, so a
//	single memmove by the right offset moves every kept tile into place; the
//	bytes that wrap around belong to chunks that get replaced below.
	if(overlap)
	{
		int offset = (dcy * CHUNK_SIZE) * WINDOW_WIDTH + dcx * CHUNK_SIZE;
		int size = WINDOW_WIDTH * WINDOW_HEIGHT;
		uint8_t *buffer = (uint8_t *)world.tiles;

		if(offset > 0) memmove(buffer, buffer + offset, size - offset);
		else if(offset < 0) memmove(buffer - offset, buffer, size + offset);
	}
	for(int sy = 0; sy < WINDOW_CHUNKS_Y; sy++)
	{
		for(int sx = 0; sx < WINDOW_CHUNKS_X; sx++)
		{
			int osx = sx + dcx, osy = sy + dcy;
			retained[sy][sx] = overlap && osx >= 0 && osx < WINDOW_CHUNKS_X && osy >= 0 && osy < WINDOW_CHUNKS_Y;
			dirty[sy][sx] = retained[sy][sx] ? stream.dirty[osy][osx] : 0;
		}
	}
	memcpy(stream.dirty, dirty, sizeof(dirty));
	stream.originX = newX;
	stream.originY = newY;

//	3. Load the new chunks that were saved, generate the others
	ioCount = 0;
	for(int sy = 0; sy < WINDOW_CHUNKS_Y; sy++)
	{
		for(int sx = 0; sx < WINDOW_CHUNKS_X; sx++)
		{
			if(retained[sy][sx]) continue;
			ioChunkX[ioCount] = (newX >> CHUNK_SHIFT) + sx;
			ioChunkY[ioCount] = (newY >> CHUNK_SHIFT) + sy;
			ioSlotX[ioCount] = sx;
			ioSlotY[ioCount] = sy;
			ioLoaded[ioCount] = false;
			ioCount++;
		}
	}
	total = ioCount;
	if(ioCount > 0) gint_world_switch(GINT_CALL(loadChunksOp));
	for(int k = 0; k < ioCount; k++)
	{
		if(ioLoaded[k]) continue;
		if(showProgress) middleText("Generating World", done * 100 / total);
		wgGenerateChunk(ioChunkX[k], ioChunkY[k], slotPtr(ioSlotX[k], ioSlotY[k]), WINDOW_WIDTH);
		done++;
	}

//	4. Keep window-local coordinates pointing at the same places
	if(haveOld) shiftObjects(oldX - newX, oldY - newY);
	return true;
}

/* Window position that puts the given tile in the second column and second
   row of chunks */
static void windowFor(int globalTileX, int globalTileY, int *x, int *y)
{
	*x = (floorDivInt(globalTileX, CHUNK_SIZE) - 1) * CHUNK_SIZE;
	*y = (floorDivInt(globalTileY, CHUNK_SIZE) - 1) * CHUNK_SIZE;
	*x = clampOrigin(*x, WORLD_LIMIT_X, WINDOW_WIDTH);
	*y = clampOrigin(*y, WORLD_LIMIT_Y, WINDOW_HEIGHT);
}

bool streamInit(int globalTileX, int globalTileY)
{
	int x, y;

	if(stream.ioBuffer == NULL) stream.ioBuffer = malloc(CHUNK_BYTES);
	if(stream.ioBuffer == NULL) return false;
	memset(stream.dirty, 0, sizeof(stream.dirty));
	stream.error = -99;
	windowFor(globalTileX, globalTileY, &x, &y);
	moveWindow(x, y, false, true);
	resetVarBuffer();
	return true;
}

bool streamUpdate()
{
	static int cooldown = 0;
	int px = (player.props.x + (player.props.width >> 1)) >> 3;
	int py = (player.props.y + (player.props.height >> 1)) >> 3;
	int dx = 0, dy = 0;
	int newX, newY;

	if(cooldown > 0)
	{
		cooldown--;
		return true;
	}

	if(px < SHIFT_MARGIN_X) dx = -1;
	else if(px >= WINDOW_WIDTH - SHIFT_MARGIN_X) dx = 1;
	if(py < SHIFT_MARGIN_Y) dy = -1;
	else if(py >= WINDOW_HEIGHT - SHIFT_MARGIN_Y) dy = 1;
	if(dx == 0 && dy == 0) return true;

	newX = clampOrigin(stream.originX + dx * CHUNK_SIZE, WORLD_LIMIT_X, WINDOW_WIDTH);
	newY = clampOrigin(stream.originY + dy * CHUNK_SIZE, WORLD_LIMIT_Y, WINDOW_HEIGHT);
//	World border
	if(newX == stream.originX && newY == stream.originY) return true;

	if(!moveWindow(newX, newY, true, false))
	{
//		Storage is probably full; don't retry every frame
		cooldown = 600;
		return false;
	}
	return true;
}

bool streamTeleport(int globalPixelX, int globalPixelY)
{
	int x, y;

	windowFor(globalPixelX >> 3, globalPixelY >> 3, &x, &y);
	if(x != stream.originX || y != stream.originY)
	{
		if(!moveWindow(x, y, true, false)) return false;
	}
	player.props.x = globalPixelX - stream.originX * 8;
	player.props.y = globalPixelY - stream.originY * 8;
	resetVarBuffer();
	return true;
}

void streamFree()
{
	free(stream.ioBuffer);
	stream.ioBuffer = NULL;
}
