#pragma once

/*
----- WORLDGEN -----

Infinite, deterministic world generation in the style of Minecraft (1.18+).

The world is an endless grid of CHUNK_SIZE x CHUNK_SIZE chunks. Every chunk is
a pure function of (world seed, chunk coordinates): generating the same chunk
twice always gives the same tiles, so only chunks the player has modified need
to be saved. Anything that crosses a chunk border (trees, caves, lakes,
structures, ore veins) is "pulled": while generating a chunk, the generator
replays every nearby feature origin and draws only the part that falls inside
the chunk.

Pipeline for one chunk (mirrors Minecraft's generation steps):
 1. Climate:    1D multi-noise (continentalness, erosion, weirdness,
                temperature, humidity) -> biome by nearest parameter box.
 2. Terrain:    height from continentalness/erosion/peaks splines + detail,
                rivers where the river noise crosses zero.
 3. Surface:    per-biome surface rules (grass/dirt, sand, mud, stone...).
 4. Caves:      noise caves (cheese, spaghetti, noodle), sampled on a coarse
                4x4 grid and interpolated, then legacy "worm" carvers.
 5. Lakes:      surface lakes, sealed so they can't drain into caves.
 6. Structures: mineshafts, buried cabins, surface houses, placed on a
                random spread grid (spacing/separation) like Minecraft.
 7. Ores:       vein features with height distributions.
 8. Decoration: gems, cobwebs, pots, life crystals, underground jungle.
 9. Vegetation: trees, cacti, plants, mushrooms, vines.

Coordinates are global tile coordinates. Y grows downwards; SEA_LEVEL is the
first row of sea water.

This file has no gint dependency except an optional scratch section, so it can
be compiled on a PC (see util/wgpreview.c).
*/

#include <stdint.h>
#include <stdbool.h>

#define CHUNK_SHIFT 5
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define SEA_LEVEL 0

enum Biomes {
	BIOME_OCEAN,
	BIOME_BEACH,
	BIOME_PLAINS,
	BIOME_FOREST,
	BIOME_DESERT,
	BIOME_JUNGLE,
	BIOME_SWAMP,
	BIOME_MOUNTAINS,

	BIOMES_COUNT
};

/* Loot categories returned by wgChestLoot(). Match enum LootTables. */
enum WorldgenLoot {
	WG_LOOT_UNDERGROUND = 0,
	WG_LOOT_SURFACE = 1
};

/* wgSetSeed
Sets the world seed used by every other function.
*/
void wgSetSeed(uint32_t seed);

uint32_t wgGetSeed(void);

/* wgSurfaceY
Returns the Y coordinate of the first solid tile of the terrain heightmap in
column gx (before caves, lakes and structures). Tiles above it are air, or
water if they are at or below SEA_LEVEL.
*/
int wgSurfaceY(int gx);

/* wgBiome
Returns the surface biome of column gx.
*/
int wgBiome(int gx);

/* wgBiomeName
Returns a printable name for a biome ID.
*/
const char *wgBiomeName(int biome);

/* wgFindSpawnX
Searches outwards from x = 0 for a dry, grassy column to spawn the player on.
*/
int wgFindSpawnX(void);

/* wgGenerateChunk
Generates chunk (cx, cy) into out[y * stride + x] for 0 <= x, y < CHUNK_SIZE.
*/
void wgGenerateChunk(int cx, int cy, uint8_t *out, int stride);

/* wgChestLoot
Chooses the loot category for a world-generated chest whose top-left tile is
at (gx, gy). Chests are filled lazily the first time they are opened.
*/
int wgChestLoot(int gx, int gy);
