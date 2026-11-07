/*
wgpreview - renders the Terrario world generator on a PC.

Build (from the repository root):
	gcc -O2 -std=c11 -I include -o wgpreview util/wgpreview.c src/worldgen.c

Usage:
	./wgpreview <seed> <x0> <y0> <width> <height> <out.ppm> [scale]

Coordinates are in tiles (rounded to whole chunks). Y grows downwards and
y = 0 is sea level. The image is a binary PPM; most image viewers open it, or
convert it with any tool you like.

It also checks that generation is deterministic (every chunk is generated
twice and compared) and prints the time per chunk.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "tiles.h"
#include "worldgen.h"

static const uint8_t colours[TILES_COUNT][3] = {
	[TILE_NOTHING] = {200, 225, 255},
	[TILE_STONE] = {120, 120, 120},
	[TILE_DIRT] = {140, 95, 55},
	[TILE_GRASS] = {60, 170, 60},
	[TILE_WOOD] = {190, 140, 80},
	[TILE_TRUNK] = {110, 70, 30},
	[TILE_ROOT_L] = {110, 70, 30},
	[TILE_ROOT_R] = {110, 70, 30},
	[TILE_LEAVES] = {20, 110, 20},
	[TILE_PLANT] = {120, 210, 90},
	[TILE_WBENCH_L] = {230, 120, 40},
	[TILE_WBENCH_R] = {230, 120, 40},
	[TILE_PLATFORM] = {230, 190, 120},
	[TILE_CHAIR_L] = {230, 120, 40},
	[TILE_CHAIR_R] = {230, 120, 40},
	[TILE_TORCH] = {255, 230, 0},
	[TILE_IRON_ORE] = {210, 170, 150},
	[TILE_CHEST_L] = {255, 0, 255},
	[TILE_CHEST_R] = {255, 0, 255},
	[TILE_DOOR_C] = {150, 80, 20},
	[TILE_VINE] = {40, 140, 40},
	[TILE_SAND] = {235, 215, 140},
	[TILE_CACTUS] = {30, 130, 70},
	[TILE_CACTUS_BRANCH] = {30, 130, 70},
	[TILE_WATER] = {40, 80, 220},
	[TILE_CRYST_L] = {255, 40, 80},
	[TILE_CRYST_R] = {255, 40, 80},
	[TILE_MUD] = {90, 60, 60},
	[TILE_CLAY] = {190, 100, 80},
	[TILE_COPPER_ORE] = {230, 120, 50},
	[TILE_TIN_ORE] = {180, 180, 150},
	[TILE_MUSHROOM] = {230, 60, 60},
	[TILE_AMETHYST] = {170, 80, 230},
	[TILE_DIAMOND] = {230, 255, 255},
	[TILE_EMERALD] = {0, 230, 110},
	[TILE_RUBY] = {230, 0, 30},
	[TILE_SAPPHIRE] = {0, 80, 255},
	[TILE_TOPAZ] = {255, 200, 0},
	[TILE_POT_L] = {160, 60, 20},
	[TILE_POT_R] = {160, 60, 20},
	[TILE_COBWEB] = {235, 235, 235},
	[TILE_GRASS_JUNGLE] = {120, 220, 30},
};

int main(int argc, char **argv)
{
	uint32_t seed;
	int x0, y0, w, h, scale = 1;
	int cx0, cy0, cw, ch;
	uint8_t *map, *check;
	FILE *f;
	clock_t start;
	long mismatches = 0;
	int counts[BIOMES_COUNT] = {0};

	if(argc < 7)
	{
		fprintf(stderr, "usage: %s seed x0 y0 width height out.ppm [scale]\n", argv[0]);
		return 1;
	}
	seed = strtoul(argv[1], NULL, 0);
	x0 = atoi(argv[2]); y0 = atoi(argv[3]);
	w = atoi(argv[4]); h = atoi(argv[5]);
	if(argc > 7) scale = atoi(argv[7]);

	cx0 = x0 >> CHUNK_SHIFT;
	cy0 = y0 >> CHUNK_SHIFT;
	cw = (w + CHUNK_SIZE - 1) / CHUNK_SIZE;
	ch = (h + CHUNK_SIZE - 1) / CHUNK_SIZE;
	w = cw * CHUNK_SIZE;
	h = ch * CHUNK_SIZE;

	wgSetSeed(seed);
	map = malloc((size_t)w * h);
	check = malloc(CHUNK_SIZE * CHUNK_SIZE);

	start = clock();
//	Generate in a scrambled order to make sure order doesn't matter
	for(int k = 0; k < cw * ch; k++)
	{
		int idx = (int)(((long)k * 7919) % (cw * ch));
		int i = idx % cw, j = idx / cw;
		wgGenerateChunk(cx0 + i, cy0 + j, map + (size_t)j * CHUNK_SIZE * w + i * CHUNK_SIZE, w);
	}
	double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
	printf("%d chunks, %.3f ms/chunk\n", cw * ch, 1000.0 * elapsed / (cw * ch));

	for(int j = 0; j < ch; j++)
	{
		for(int i = 0; i < cw; i++)
		{
			wgGenerateChunk(cx0 + i, cy0 + j, check, CHUNK_SIZE);
			for(int y = 0; y < CHUNK_SIZE; y++)
				mismatches += memcmp(check + y * CHUNK_SIZE, map + ((size_t)j * CHUNK_SIZE + y) * w + i * CHUNK_SIZE, CHUNK_SIZE) != 0;
		}
	}
	printf("determinism: %ld mismatching rows\n", mismatches);

	for(int x = 0; x < w; x++) counts[wgBiome(cx0 * CHUNK_SIZE + x)]++;
	for(int b = 0; b < BIOMES_COUNT; b++) printf("%-10s %5.1f%%\n", wgBiomeName(b), 100.0 * counts[b] / w);
	printf("spawn x = %d\n", wgFindSpawnX());

	f = fopen(argv[6], "wb");
	fprintf(f, "P6 %d %d 255\n", w * scale, h * scale);
	for(int y = 0; y < h * scale; y++)
	{
		for(int x = 0; x < w * scale; x++)
		{
			uint8_t t = map[(size_t)(y / scale) * w + x / scale];
			const uint8_t *c = t < TILES_COUNT ? colours[t] : (const uint8_t[]){255, 0, 0};
			int gy = cy0 * CHUNK_SIZE + y / scale;
			uint8_t px[3] = {c[0], c[1], c[2]};
//			Darken the sky a little below sea level so it reads as underground air
			if(t == TILE_NOTHING && gy > wgSurfaceY(cx0 * CHUNK_SIZE + x / scale) + 2)
			{
				px[0] = 40; px[1] = 30; px[2] = 25;
			}
			fwrite(px, 1, 3, f);
		}
	}
	fclose(f);
	free(map);
	free(check);
	return 0;
}
