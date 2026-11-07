/*
----- WORLDGEN -----

Minecraft-style infinite world generation. See worldgen.h for an overview.

Everything here is integer/fixed point: the FX build uses -m3 (no FPU), so
float maths would be emulated and far too slow to generate chunks while the
player walks. Values called "Q12" are scaled by 4096, "Q11" values are in
[-2048, 2047] and represent [-1, 1].
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "tiles.h"
#include "worldgen.h"

/* Scratch buffers only live during one wgGenerateChunk() call, which never
   switches to the OS world, so on the calculator they can sit in on-chip
   memory instead of the scarce heap. */
#if defined(FX9860G) || defined(FXCG50)
#include <gint/defs/attributes.h>
#define WG_SCRATCH GYRAM
#else
#define WG_SCRATCH
#endif

#define CS CHUNK_SIZE
#define ONE 4096

#define RIVER_WIDTH 70
#define PROTECT_RADIUS 5
#define COL_MARGIN 6
#define WORM_RANGE 3

enum Salts {
	SALT_CONT = 0x100, SALT_EROS, SALT_WEIRD, SALT_TEMP, SALT_HUMID, SALT_RIVER,
	SALT_DETAIL, SALT_JITTER, SALT_RIDGE, SALT_DIRT, SALT_FLOOR,
	SALT_CHEESE, SALT_CHEESE2, SALT_SPAG, SALT_SPAGW, SALT_NOODLE, SALT_TOGGLE,
	SALT_JUNGLE, SALT_WORM, SALT_WORM2, SALT_LAKE, SALT_MINE, SALT_CABIN,
	SALT_HOUSE, SALT_ORE, SALT_GEM, SALT_WEB, SALT_POT, SALT_CRYST, SALT_TREE,
	SALT_CACTUS, SALT_PLANT, SALT_VINE, SALT_TORCH, SALT_JPLANT
};

static uint32_t worldSeed;

void wgSetSeed(uint32_t seed) { worldSeed = seed; }
uint32_t wgGetSeed(void) { return worldSeed; }

//---
// Hashing and random numbers
//---

static inline uint32_t mix32(uint32_t v)
{
	v ^= v >> 16;
	v *= 0x7feb352dU;
	v ^= v >> 15;
	v *= 0x846ca68bU;
	v ^= v >> 16;
	return v;
}

static inline uint32_t hash1(int32_t x, uint32_t salt)
{
	return mix32((uint32_t)x * 0x9E3779B1U + mix32(worldSeed ^ salt));
}

static inline uint32_t hash2(int32_t x, int32_t y, uint32_t salt)
{
	return mix32((uint32_t)x * 0x9E3779B1U
		+ mix32((uint32_t)y * 0x85EBCA77U + mix32(worldSeed ^ salt)));
}

/* Small counter-based generator. Every feature seeds its own Rng from its
   origin, so skipping a feature never shifts the random numbers of another. */
typedef struct { uint32_t s; } Rng;

static inline Rng rngAt(int32_t x, int32_t y, uint32_t salt)
{
	Rng r = { hash2(x, y, salt) };
	return r;
}

static inline uint32_t rngNext(Rng *r)
{
	r->s += 0x9E3779B9U;
	return mix32(r->s);
}

/* Uniform integer in [0, n) */
static inline int rngInt(Rng *r, int n)
{
	if(n <= 1) return 0;
	return (int)(rngNext(r) % (uint32_t)n);
}

/* Uniform integer in [lo, hi] */
static inline int rngRange(Rng *r, int lo, int hi)
{
	return lo + rngInt(r, hi - lo + 1);
}

static inline int32_t floorDiv(int32_t a, int32_t b)
{
	return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static inline int iabs(int v) { return v < 0 ? -v : v; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int clampQ11(int v) { return v < -2048 ? -2048 : (v > 2047 ? 2047 : v); }

static const int16_t sinTab[256] = {
	   0,    6,   13,   19,   25,   31,   38,   44,   50,   56,   62,   68,   74,   80,   86,   92,
	  98,  104,  109,  115,  121,  126,  132,  137,  142,  147,  152,  157,  162,  167,  172,  177,
	 181,  185,  190,  194,  198,  202,  206,  209,  213,  216,  220,  223,  226,  229,  231,  234,
	 237,  239,  241,  243,  245,  247,  248,  250,  251,  252,  253,  254,  255,  255,  256,  256,
	 256,  256,  256,  255,  255,  254,  253,  252,  251,  250,  248,  247,  245,  243,  241,  239,
	 237,  234,  231,  229,  226,  223,  220,  216,  213,  209,  206,  202,  198,  194,  190,  185,
	 181,  177,  172,  167,  162,  157,  152,  147,  142,  137,  132,  126,  121,  115,  109,  104,
	  98,   92,   86,   80,   74,   68,   62,   56,   50,   44,   38,   31,   25,   19,   13,    6,
	   0,   -6,  -13,  -19,  -25,  -31,  -38,  -44,  -50,  -56,  -62,  -68,  -74,  -80,  -86,  -92,
	 -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
	-181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
	-237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
	-256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
	-237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
	-181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
	 -98,  -92,  -86,  -80,  -74,  -68,  -62,  -56,  -50,  -44,  -38,  -31,  -25,  -19,  -13,   -6,
};

#define SIN8(a) (sinTab[(a) & 255])
#define COS8(a) (sinTab[((a) + 64) & 255])

//---
// Noise
//---

/* Cubic smoothstep, t in Q12 */
static inline int32_t smooth3(int32_t t)
{
	return (((t * t) >> 12) * (3 * ONE - 2 * t)) >> 12;
}

/* Perlin's quintic fade, t in Q12 */
static inline int32_t fade5(int32_t t)
{
	int32_t t3 = (((t * t) >> 12) * t) >> 12;
	return (t3 * (((t * (t * 6 - 15 * ONE)) >> 12) + 10 * ONE)) >> 12;
}

/* 1D value noise with a wavelength of 2^shift tiles. Returns [0, 4095]. */
static int vnoise1(int32_t x, int shift, uint32_t salt)
{
	int32_t cell = x >> shift;
	int32_t t = (x - cell * (1 << shift)) << (12 - shift);
	int32_t a = (int32_t)(hash1(cell, salt) & 4095);
	int32_t b = (int32_t)(hash1(cell + 1, salt) & 4095);

	return a + (((b - a) * smooth3(t)) >> 12);
}

/* Fractal value noise, normalised back to [0, 4095]. */
static int fbm1(int32_t x, int shift, int octaves, uint32_t salt)
{
	int sum = 0, norm = 0, amp = ONE;

	for(int i = 0; i < octaves && shift >= 1; i++)
	{
		sum += (vnoise1(x, shift, salt + i * 0x9E37U) * amp) >> 12;
		norm += amp;
		amp >>= 1;
		shift--;
	}
	return sum * ONE / norm;
}

/* 2D gradient (Perlin) noise with a wavelength of 2^shift tiles.
   Returns roughly [-4096, 4096]. */
static int pnoise2(int32_t x, int32_t y, int shift, uint32_t salt)
{
	static const int8_t grads[8][2] = {
		{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
	};
	int32_t cx = x >> shift, cy = y >> shift;
	int32_t fx = (x - cx * (1 << shift)) << (12 - shift);
	int32_t fy = (y - cy * (1 << shift)) << (12 - shift);
	int32_t u = fade5(fx), v = fade5(fy);
	const int8_t *g00 = grads[hash2(cx, cy, salt) & 7];
	const int8_t *g10 = grads[hash2(cx + 1, cy, salt) & 7];
	const int8_t *g01 = grads[hash2(cx, cy + 1, salt) & 7];
	const int8_t *g11 = grads[hash2(cx + 1, cy + 1, salt) & 7];
	int32_t d00 = g00[0] * fx + g00[1] * fy;
	int32_t d10 = g10[0] * (fx - ONE) + g10[1] * fy;
	int32_t d01 = g01[0] * fx + g01[1] * (fy - ONE);
	int32_t d11 = g11[0] * (fx - ONE) + g11[1] * (fy - ONE);
	int32_t x0 = d00 + (((d10 - d00) * u) >> 12);
	int32_t x1 = d01 + (((d11 - d01) * u) >> 12);

	return x0 + (((x1 - x0) * v) >> 12);
}

//---
// Climate and terrain shape
//---

typedef struct {
	int c;	// Continentalness: ocean -> coast -> inland
	int e;	// Erosion: low = mountains, high = flat
	int w;	// Weirdness, folded into peaks/valleys
	int t;	// Temperature
	int h;	// Humidity
	int r;	// River noise, rivers where it crosses zero
} Climate;

/* Re-centres a [0, 4095] fbm value on 0 and stretches it (gain in Q8), since
   fractal noise rarely reaches its extremes. */
static inline int centred(int v, int gainQ8)
{
	return clampQ11(((v - 2048) * gainQ8) >> 8);
}

static void climateAt(int32_t gx, Climate *cl)
{
	cl->c = clampQ11(centred(fbm1(gx, 10, 3, SALT_CONT), 360) + 320);
	cl->e = centred(fbm1(gx, 8, 2, SALT_EROS), 360);
	cl->w = centred(fbm1(gx, 7, 2, SALT_WEIRD), 400);
	cl->t = centred(fbm1(gx, 9, 2, SALT_TEMP), 360);
	cl->h = centred(fbm1(gx, 9, 2, SALT_HUMID), 360);
	cl->r = centred(fbm1(gx, 9, 2, SALT_RIVER), 512);
}

typedef struct { int16_t x, y; } SplinePoint;

static int spline(const SplinePoint *pts, int n, int x)
{
	if(x <= pts[0].x) return pts[0].y;
	for(int i = 0; i < n - 1; i++)
	{
		if(x < pts[i + 1].x)
		{
			return pts[i].y + (pts[i + 1].y - pts[i].y) * (x - pts[i].x)
				/ (pts[i + 1].x - pts[i].x);
		}
	}
	return pts[n - 1].y;
}

/* Continentalness -> base height above sea level, in 1/16 tiles */
static const SplinePoint continentSpline[] = {
	{-2048, -40 * 16}, {-1100, -28 * 16}, {-750, -14 * 16}, {-550, -5 * 16},
	{-420, -1 * 16}, {-320, 2 * 16}, {-120, 5 * 16}, {300, 9 * 16},
	{900, 15 * 16}, {1500, 21 * 16}, {2047, 27 * 16}
};

/* Erosion -> amplitude of the local hills, in 1/16 tiles */
static const SplinePoint erosionSpline[] = {
	{-2048, 14 * 16}, {-600, 10 * 16}, {0, 6 * 16}, {800, 3 * 16}, {2047, 2 * 16}
};

#define SPLINE_LEN(s) ((int)(sizeof(s) / sizeof(s[0])))

/* Height of the terrain above sea level, in 1/16 tiles */
static int heightAt(int32_t gx, const Climate *cl)
{
	int h = spline(continentSpline, SPLINE_LEN(continentSpline), cl->c);
	int amp = spline(erosionSpline, SPLINE_LEN(erosionSpline), cl->e);
	int detail = centred(fbm1(gx, 5, 3, SALT_DETAIL), 512);

	h += (detail * amp) >> 11;
	h += (vnoise1(gx, 2, SALT_JITTER) - 2048) >> 7;

//	Peaks: low erosion inland, shaped by the folded weirdness like Minecraft's
//	"peaks and valleys" parameter
	if(cl->e < -600 && cl->c > -450)
	{
//		Fade the peaks in from the coast instead of cutting them off
		int coast = imin(cl->c + 450, 350);
		int m = imin(-600 - cl->e, 1448) * coast / 350;
		int pv = 2048 - iabs(3 * iabs(cl->w) - 4096);
		int peaks;
		int ridge = centred(fbm1(gx, 3, 2, SALT_RIDGE), 512);

		if(pv < -1024) pv = -1024;
		peaks = (m * (pv + 2048)) >> 12;
		h += peaks * 50 * 16 / 1448;
		h += (ridge * m / 1448 * 4 * 16) >> 11;
	}

//	Rivers carve a valley down to just below sea level
	if(cl->c > -480)
	{
		int rv = iabs(cl->r);
		if(rv < RIVER_WIDTH)
		{
			int f = smooth3((RIVER_WIDTH - rv) * ONE / RIVER_WIDTH);
			f = f * imin(cl->c + 480, 160) / 160;
			int target = -3 * 16;
			if(h > target) h += ((target - h) * f) >> 12;
		}
	}

	return h;
}

typedef struct {
	uint8_t biome;
	int16_t t0, t1, h0, h1, c0, c1, e0, e1;
} BiomeBox;

/* Parameter boxes, like Minecraft's multi-noise biome source: the biome whose
   box is closest to the climate point wins (earlier entries win ties). */
static const BiomeBox biomeBoxes[] = {
//	 Biome				Temperature		Humidity		Continentalness	Erosion
	{BIOME_OCEAN,		-2048, 2047,	-2048, 2047,	-2048, -520,	-2048, 2047},
	{BIOME_BEACH,		-2048, 2047,	-2048, 2047,	-520, -330,		-700, 2047},
	{BIOME_MOUNTAINS,	-2048, 2047,	-2048, 2047,	0, 2047,		-2048, -1000},
	{BIOME_DESERT,		450, 2047,		-2048, -150,	-330, 2047,		-2048, 2047},
	{BIOME_JUNGLE,		250, 2047,		450, 2047,		-330, 2047,		-2048, 2047},
	{BIOME_SWAMP,		-900, 600,		450, 2047,		-330, 900,		150, 2047},
	{BIOME_FOREST,		-2048, 450,		0, 2047,		-330, 2047,		-2048, 2047},
	{BIOME_PLAINS,		-2048, 450,		-2048, 0,		-330, 2047,		-2048, 2047},
};

static inline int boxDistance(int v, int lo, int hi)
{
	return v < lo ? lo - v : (v > hi ? v - hi : 0);
}

static int pickBiome(const Climate *cl, int h)
{
	int best = BIOME_PLAINS;
	int32_t bestFit = INT32_MAX;

	for(unsigned int i = 0; i < sizeof(biomeBoxes) / sizeof(biomeBoxes[0]); i++)
	{
		const BiomeBox *b = &biomeBoxes[i];
		int dt = boxDistance(cl->t, b->t0, b->t1);
		int dh = boxDistance(cl->h, b->h0, b->h1);
		int dc = boxDistance(cl->c, b->c0, b->c1);
		int de = boxDistance(cl->e, b->e0, b->e1);
		int32_t fit = dt * dt + dh * dh + dc * dc + de * de;
		if(fit < bestFit)
		{
			bestFit = fit;
			best = b->biome;
		}
	}

//	Islands and cliffs: a beach or ocean that ends up high above the sea
	if((best == BIOME_OCEAN && h > 2 * 16) || (best == BIOME_BEACH && h > 7 * 16))
		best = cl->h > 0 ? BIOME_FOREST : BIOME_PLAINS;
	return best;
}

static void evalColumn(int32_t gx, int *surf, int *biome, Climate *cl)
{
	int h;

	climateAt(gx, cl);
	h = heightAt(gx, cl);
	*surf = SEA_LEVEL - (h >> 4);
	*biome = pickBiome(cl, h);
}

int wgSurfaceY(int gx)
{
	Climate cl;
	int surf, biome;

	evalColumn(gx, &surf, &biome, &cl);
	return surf;
}

int wgBiome(int gx)
{
	Climate cl;
	int surf, biome;

	evalColumn(gx, &surf, &biome, &cl);
	return biome;
}

const char *wgBiomeName(int biome)
{
	static const char *names[] = {
		"Ocean", "Beach", "Plains", "Forest", "Desert", "Jungle", "Swamp", "Mountains"
	};
	if(biome < 0 || biome >= BIOMES_COUNT) return "?";
	return names[biome];
}

int wgChestLoot(int gx, int gy)
{
	return gy > wgSurfaceY(gx) + 12 ? WG_LOOT_UNDERGROUND : WG_LOOT_SURFACE;
}

//---
// Per-chunk state
//---

enum ColumnFlags {
	COL_STONE_TOP = 1,
	COL_GRASS = 2,		// Surface tile is grass/jungle grass
	COL_DRY = 4			// Surface is above sea level
};

typedef struct {
	int16_t surf;
	int16_t protectY;	// No caves above this row (keeps water from draining)
	int16_t jungle;		// Underground jungle strength, Q11
	uint8_t biome;
	uint8_t flags;
	uint8_t dirtDepth;
} Column;

/* Columns -COL_MARGIN .. CS - 1 + COL_MARGIN of the chunk being generated */
#define NUM_COLS (CS + 2 * COL_MARGIN)
static WG_SCRATCH Column cols[NUM_COLS];

/* Noise cave fields sampled every 4 tiles, see caveFields() */
#define NUM_FIELDS 6
#define GRID (CS / 4 + 1)
static WG_SCRATCH int16_t grid[NUM_FIELDS][GRID][GRID];

enum PieceTypes {
	PIECE_ROOM,
	PIECE_CORRIDOR,
	PIECE_SHAFT,
	PIECE_CABIN,
	PIECE_HOUSE
};

enum PieceFlags {
	PF_CHEST = 1,
	PF_DOOR_RIGHT = 2,
	PF_PLATFORM = 4,
	PF_POT = 8
};

typedef struct {
	int32_t x0, y0, x1, y1;
	int16_t a, b, c;
	uint8_t type, flags;
} Piece;

#define MAX_PIECES 40
static WG_SCRATCH Piece pieces[MAX_PIECES];
static int numPieces;

typedef struct { int32_t x0, x1; } Span;

#define MAX_SPANS 6
static WG_SCRATCH Span lakeSpans[MAX_SPANS];
static WG_SCRATCH Span houseSpans[MAX_SPANS];
static int numLakeSpans, numHouseSpans;

static uint8_t *chunkOut;
static int chunkStride;
static int32_t chunkX0, chunkY0;

static inline Column *colAt(int32_t gx)
{
	return &cols[gx - chunkX0 + COL_MARGIN];
}

static inline bool inChunk(int32_t gx, int32_t gy)
{
	return (uint32_t)(gx - chunkX0) < CS && (uint32_t)(gy - chunkY0) < CS;
}

static inline uint8_t getT(int32_t gx, int32_t gy)
{
	return chunkOut[(gy - chunkY0) * chunkStride + (gx - chunkX0)];
}

static inline void setT(int32_t gx, int32_t gy, uint8_t tile)
{
	chunkOut[(gy - chunkY0) * chunkStride + (gx - chunkX0)] = tile;
}

static inline void put(int32_t gx, int32_t gy, uint8_t tile)
{
	if(inChunk(gx, gy)) setT(gx, gy, tile);
}

static inline bool isSolid(uint8_t t)
{
	switch(t)
	{
		case TILE_STONE: case TILE_DIRT: case TILE_GRASS: case TILE_WOOD:
		case TILE_SAND: case TILE_MUD: case TILE_CLAY: case TILE_IRON_ORE:
		case TILE_COPPER_ORE: case TILE_TIN_ORE: case TILE_GRASS_JUNGLE:
			return true;
		default:
			return false;
	}
}

/* Sand falls, so keep caves clear of the sand layer of sandy biomes */
static inline int sandRoof(const Column *col)
{
	return (col->biome == BIOME_DESERT || col->biome == BIOME_BEACH || col->biome == BIOME_OCEAN) ? 12 : 0;
}

/* Carves a tile out of the terrain, unless that would let water drain */
static inline void carve(int32_t gx, int32_t gy)
{
	Column *col;
	uint8_t t;

	if(!inChunk(gx, gy)) return;
	col = colAt(gx);
//	Carvers never break the surface (trees and houses stand on it)
	if(gy < col->protectY || gy - col->surf < imax(3, sandRoof(col))) return;
	t = getT(gx, gy);
	if(t != TILE_WATER) setT(gx, gy, TILE_NOTHING);
}

//---
// Caves
//---

static void caveFields(int32_t gx, int32_t gy, int16_t *f)
{
	int32_t sy = gy + (gy >> 1);	// Squash vertically: caves run sideways

	f[0] = (pnoise2(gx, sy, 6, SALT_CHEESE) + (pnoise2(gx, sy, 5, SALT_CHEESE2) >> 1));
	f[1] = pnoise2(gx, sy, 7, SALT_SPAG);
	f[2] = pnoise2(gx, gy, 7, SALT_SPAGW);
	f[3] = pnoise2(gx, sy, 5, SALT_NOODLE);
	f[4] = pnoise2(gx, gy, 7, SALT_TOGGLE);
	f[5] = pnoise2(gx, gy, 5, SALT_JUNGLE);
}

/* Bilinear interpolation of the 4x4-tile cave grid, identical whether the
   corners come from the chunk cache or are computed on the spot. */
static inline int lerpCell(int v00, int v10, int v01, int v11, int fx, int fy)
{
	return (v00 * (4 - fx) * (4 - fy) + v10 * fx * (4 - fy)
		+ v01 * (4 - fx) * fy + v11 * fx * fy) >> 4;
}

/* Minecraft 1.18-style noise caves:
   - cheese: large caverns where a noise is high, absent near the surface;
   - spaghetti: long tunnels along the zero contour of a noise, which may
     break through the surface as cave entrances;
   - noodle: thin, wiggly tunnels in toggled-on regions. */
static bool caveTest(const int *f, int depth)
{
	if(depth >= 12)
	{
		int thr = depth >= 60 ? 1500 : 2600 - (depth - 12) * 1100 / 48;
		if(f[0] > thr) return true;
	}

	if(depth >= 5 || (depth >= 0 && f[4] > 1700))
	{
		int w = 110 + ((f[2] * 90) >> 12);
		if(w < 45) w = 45;
		if(iabs(f[1]) < w) return true;
	}

	if(depth >= 10 && f[4] < -600 && iabs(f[3]) < 150) return true;

	return false;
}

/* Whether the noise caves remove the tile at (gx, gy) (pure). */
static bool noiseCaveAt(int32_t gx, int32_t gy, int depth)
{
	int16_t c00[NUM_FIELDS], c10[NUM_FIELDS], c01[NUM_FIELDS], c11[NUM_FIELDS];
	int32_t bx = gx & ~3, by = gy & ~3;
	int fx = gx & 3, fy = gy & 3;
	int f[NUM_FIELDS];

	caveFields(bx, by, c00);
	caveFields(bx + 4, by, c10);
	caveFields(bx, by + 4, c01);
	caveFields(bx + 4, by + 4, c11);
	for(int i = 0; i < NUM_FIELDS; i++) f[i] = lerpCell(c00[i], c10[i], c01[i], c11[i], fx, fy);
	return caveTest(f, depth);
}

static void carveDisc(int32_t cx, int32_t cy, int rQ4)
{
	int r = (rQ4 >> 4) + 1;
	int32_t r2 = rQ4 * rQ4;

	if(cx + r < chunkX0 || cx - r >= chunkX0 + CS || cy + r < chunkY0 || cy - r >= chunkY0 + CS) return;
	for(int dy = -r; dy <= r; dy++)
	{
		for(int dx = -r; dx <= r; dx++)
		{
			int32_t gx = cx + dx, gy = cy + dy;
			if(dx * dx * 256 + dy * dy * 256 > r2) continue;
			if(!inChunk(gx, gy)) continue;
			if(gy - colAt(gx)->surf < 3) continue;
			carve(gx, gy);
		}
	}
}

/* Legacy Minecraft cave carver: random-walk tunnels that start in nearby
   chunks and are replayed for every chunk they might cross. */
static void carveWorms(int cx, int cy)
{
	for(int scy = cy - WORM_RANGE; scy <= cy + WORM_RANGE; scy++)
	{
		for(int scx = cx - WORM_RANGE; scx <= cx + WORM_RANGE; scx++)
		{
			Rng r = rngAt(scx, scy, SALT_WORM);
			int count;

			if(rngInt(&r, 100) >= 12) continue;
			count = 1 + (rngInt(&r, 3) == 0);

			for(int k = 0; k < count; k++)
			{
				Rng w = rngAt(scx * 4 + k, scy, SALT_WORM2);
				int32_t sx = scx * CS + rngInt(&w, CS);
				int32_t sy = scy * CS + rngInt(&w, CS);
				int len = 40 + rngInt(&w, 50);
				int32_t x8 = sx * 256 + 128, y8 = sy * 256 + 128;
				int ang = rngInt(&w, 256), angVel = 0;
				int baseR = 16 + rngInt(&w, 24);	// Q4 tiles
				int bulge = rngInt(&w, 32);

//				Quick reject: the worm can't reach this chunk
				if(sx + len + 6 < chunkX0 || sx - len - 6 >= chunkX0 + CS
				|| sy + len + 6 < chunkY0 || sy - len - 6 >= chunkY0 + CS) continue;
				if(sy < wgSurfaceY(sx) + 10) continue;

//				Sometimes open with a cave room
				if(rngInt(&w, 4) == 0)
				{
					int rx = rngRange(&w, 3, 6), ry = rngRange(&w, 2, 4);
					for(int dy = -ry; dy <= ry; dy++)
					{
						for(int dx = -rx; dx <= rx; dx++)
						{
							if(dx * dx * ry * ry + dy * dy * rx * rx > rx * rx * ry * ry) continue;
							if(inChunk(sx + dx, sy + dy) && sy + dy - colAt(sx + dx)->surf >= 3) carve(sx + dx, sy + dy);
						}
					}
				}

				for(int step = 0; step < len; step++)
				{
					int rad = baseR + ((SIN8(step * 128 / len) * bulge) >> 8);
					carveDisc(x8 >> 8, y8 >> 8, rad);

					x8 += COS8(ang);
					y8 += SIN8(ang) >> 1;
					angVel = (angVel * 3) / 4 + rngInt(&w, 25) - 12;
					ang += angVel / 2;
				}
			}
		}
	}
}

//---
// Lakes
//---

typedef struct { int32_t x; int rad, depth, level; } Lake;

static bool lakeAt(int scx, Lake *lake)
{
	Rng r = rngAt(scx, 0, SALT_LAKE);
	int biome, sL, sR, sC;

	if(rngInt(&r, 100) >= 24) return false;
	lake->x = scx * CS + rngInt(&r, CS);
	lake->rad = rngRange(&r, 4, 11);
	lake->depth = rngRange(&r, 2, 5);
	biome = wgBiome(lake->x);
	if(biome == BIOME_OCEAN || biome == BIOME_BEACH || biome == BIOME_MOUNTAINS) return false;
	if(biome == BIOME_DESERT && rngInt(&r, 3) != 0) return false;

	sL = wgSurfaceY(lake->x - lake->rad);
	sR = wgSurfaceY(lake->x + lake->rad);
	sC = wgSurfaceY(lake->x);
	lake->level = imax(sL, sR);
	if(lake->level >= SEA_LEVEL - 1) return false;
	if(sC < lake->level - 4) return false;
	return true;
}

static void drawLakes(int cx)
{
	Lake lake;

	numLakeSpans = 0;
	for(int scx = cx - 1; scx <= cx + 1; scx++)
	{
		if(!lakeAt(scx, &lake)) continue;
		if(numLakeSpans < MAX_SPANS) lakeSpans[numLakeSpans++] = (Span){lake.x - lake.rad - 1, lake.x + lake.rad + 1};

		for(int32_t gx = lake.x - lake.rad - 1; gx <= lake.x + lake.rad + 1; gx++)
		{
			int dx = gx - lake.x;
			int bottom;

			if(gx < chunkX0 || gx >= chunkX0 + CS) continue;
			if(iabs(dx) > lake.rad)
			{
//				Side walls
				for(int32_t gy = lake.level; gy <= lake.level + 2; gy++)
				{
					if(inChunk(gx, gy) && !isSolid(getT(gx, gy))) setT(gx, gy, TILE_DIRT);
				}
				continue;
			}
			bottom = lake.level + lake.depth - lake.depth * dx * dx / (lake.rad * lake.rad);
			for(int32_t gy = lake.level; gy < bottom; gy++) put(gx, gy, TILE_WATER);
//			Floor, thick enough that the steep rim can't leak sideways into caves
			for(int32_t gy = bottom; gy <= bottom + 3; gy++)
			{
				if(inChunk(gx, gy) && !isSolid(getT(gx, gy))) setT(gx, gy, TILE_DIRT);
			}
		}
	}
}

//---
// Structures
//---

static bool piecesIntersect(const Piece *a, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	return a->x0 - 1 <= x1 && a->x1 + 1 >= x0 && a->y0 - 1 <= y1 && a->y1 + 1 >= y0;
}

static bool addPiece(int parent, uint8_t type, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	if(numPieces >= MAX_PIECES) return false;
	for(int i = 0; i < numPieces; i++)
	{
		if(i == parent) continue;
		if(piecesIntersect(&pieces[i], x0, y0, x1, y1)) return false;
	}
	pieces[numPieces] = (Piece){x0, y0, x1, y1, -1, -1, 0, type, 0};
	numPieces++;
	return true;
}

/* Random spread placement (Minecraft's structure grid): one attempt per
   spacing x spacing chunk region, at a random chunk that keeps at least
   `separation` chunks from the neighbouring regions' attempts. */
static void structureStart(int rx, int ry, int spacing, int separation, uint32_t salt, int32_t *sx, int32_t *sy, Rng *rng)
{
	*rng = rngAt(rx, ry, salt);
	*sx = (rx * spacing + rngInt(rng, spacing - separation)) * CS + rngInt(rng, CS);
	*sy = (ry * spacing + rngInt(rng, spacing - separation)) * CS + rngInt(rng, CS);
}

typedef struct { int32_t x, floor; int8_t dir; uint8_t depth; int8_t parent; } Exit;

/* Mineshaft: a tree of corridors, vertical shafts and rooms, grown from a
   start room like Minecraft's MineshaftPieces. */
static void buildMineshaft(int32_t sx, int32_t sy, Rng *r)
{
	Exit queue[24];
	int head = 0, tail = 0;
	int w = rngRange(r, 9, 15), h = rngRange(r, 5, 7);
	int32_t x0 = sx - w / 2;

	numPieces = 0;
	addPiece(-1, PIECE_ROOM, x0, sy - h + 1, x0 + w - 1, sy);
	queue[tail++] = (Exit){x0 - 1, sy, -1, 0, 0};
	queue[tail++] = (Exit){x0 + w, sy, 1, 0, 0};

	while(head < tail && numPieces < MAX_PIECES)
	{
		Exit e = queue[head++];
		int len = rngRange(r, 10, 34);
		int32_t cx0, cx1, end;
		int choice;
		int parent;

		if(e.depth > 6) continue;
		cx0 = e.dir > 0 ? e.x : e.x - len + 1;
		cx1 = e.dir > 0 ? e.x + len - 1 : e.x;
		if(iabs(cx0 - sx) > 100 || iabs(cx1 - sx) > 100 || iabs(e.floor - sy) > 70) continue;
		if(!addPiece(e.parent, PIECE_CORRIDOR, cx0, e.floor - 3, cx1, e.floor)) continue;
		parent = numPieces - 1;
		if(rngInt(r, 4) == 0)
		{
			pieces[parent].flags |= PF_CHEST;
			pieces[parent].a = rngRange(r, cx0 + 1, cx1 - 2);
		}
		end = e.dir > 0 ? cx1 + 1 : cx0 - 1;

		choice = rngInt(r, 100);
		if(choice < 40)
		{
			if(tail < 24) queue[tail++] = (Exit){end, e.floor, e.dir, e.depth + 1, parent};
		}
		else if(choice < 75)
		{
//			Vertical shaft at the end of the corridor, going up or down
			int slen = rngRange(r, 8, 20);
			bool down = rngInt(r, 3) != 0;
			int32_t shx0 = e.dir > 0 ? cx1 - 2 : cx0;
			int32_t top = down ? e.floor - 3 : e.floor - slen;
			int32_t bottom = down ? e.floor + slen : e.floor;
			int32_t newFloor = down ? bottom : top + 3;

			if(!addPiece(parent, PIECE_SHAFT, shx0, top, shx0 + 2, bottom)) continue;
			if(tail < 23)
			{
				queue[tail++] = (Exit){shx0 - 1, newFloor, -1, e.depth + 1, numPieces - 1};
				queue[tail++] = (Exit){shx0 + 3, newFloor, 1, e.depth + 1, numPieces - 1};
			}
		}
		else if(choice < 88)
		{
			int rw = rngRange(r, 5, 9), rh = rngRange(r, 4, 6);
			int32_t rx0 = e.dir > 0 ? end : end - rw + 1;
			if(!addPiece(parent, PIECE_ROOM, rx0, e.floor - rh + 1, rx0 + rw - 1, e.floor)) continue;
			if(rngInt(r, 2) && tail < 24)
				queue[tail++] = (Exit){e.dir > 0 ? rx0 + rw : rx0 - 1, e.floor, e.dir, e.depth + 1, numPieces - 1};
		}
	}
}

static void drawMinePiece(const Piece *p)
{
	switch(p->type)
	{
		case PIECE_ROOM:
			for(int32_t y = p->y0; y <= p->y1; y++)
				for(int32_t x = p->x0; x <= p->x1; x++) carve(x, y);
//			Dirt floor, like Minecraft's mineshaft start room
			for(int32_t x = p->x0; x <= p->x1; x++)
				if(inChunk(x, p->y1 + 1) && !isSolid(getT(x, p->y1 + 1))) setT(x, p->y1 + 1, TILE_DIRT);
			break;

		case PIECE_CORRIDOR:
			for(int32_t y = p->y0; y <= p->y1; y++)
			{
				for(int32_t x = p->x0; x <= p->x1; x++)
				{
					carve(x, y);
					if(y <= p->y0 + 1 && (hash2(x, y, SALT_WEB) % 22) == 0) put(x, y, TILE_COBWEB);
				}
			}
//			Support beams in the ceiling, with the odd torch hanging below
			for(int32_t x = p->x0 + 2; x <= p->x1 - 1; x += 5)
			{
				for(int dx = -1; dx <= 1; dx++)
				{
					if(inChunk(x + dx, p->y0 - 1) && isSolid(getT(x + dx, p->y0 - 1))) setT(x + dx, p->y0 - 1, TILE_WOOD);
				}
				if(hash2(x, p->y0, SALT_TORCH) % 3 == 0) put(x, p->y0, TILE_TORCH);
			}
//			Bridge over caves
			for(int32_t x = p->x0; x <= p->x1; x++)
			{
				if(inChunk(x, p->y1 + 1) && !isSolid(getT(x, p->y1 + 1))) setT(x, p->y1 + 1, TILE_PLATFORM);
			}
			if(p->flags & PF_CHEST)
			{
				int32_t x = p->a;
				put(x, p->y1 - 1, TILE_CHEST_L); put(x + 1, p->y1 - 1, TILE_CHEST_R);
				put(x, p->y1, TILE_CHEST_L); put(x + 1, p->y1, TILE_CHEST_R);
				for(int dx = 0; dx < 2; dx++)
					if(inChunk(x + dx, p->y1 + 1) && !isSolid(getT(x + dx, p->y1 + 1))) setT(x + dx, p->y1 + 1, TILE_WOOD);
			}
			break;

		case PIECE_SHAFT:
			for(int32_t y = p->y0; y <= p->y1; y++)
			{
				for(int32_t x = p->x0; x <= p->x1; x++)
				{
					carve(x, y);
//					Platforms work as a ladder
					if((y - p->y0) % 4 == 3 && y < p->y1 - 1) put(x, y, TILE_PLATFORM);
				}
			}
			break;
	}
}

/* Buried cabin, as in Terraria's original generator: 1-3 stacked wooden rooms
   with a door, platforms between floors and one chest. */
static void buildCabin(int32_t sx, int32_t sy, Rng *r)
{
	int num = rngRange(r, 1, 3);
	int chestRoom = rngInt(r, num);
	int32_t x = sx, y = sy;

	numPieces = 0;
	for(int room = 0; room < num && numPieces < MAX_PIECES; room++)
	{
		int width = rngRange(r, 10, 19);
		Piece *p = &pieces[numPieces++];

		*p = (Piece){x, y, x + width - 1, y + 5, -1, -1, 0, PIECE_CABIN, 0};
		if(rngInt(r, 2)) p->flags |= PF_DOOR_RIGHT;
		if(room > 0)
		{
			p->flags |= PF_PLATFORM;
			p->b = rngRange(r, x + 1, x + width - 5);
			p->c = rngRange(r, 2, 4);
		}
		if(room == chestRoom)
		{
			p->flags |= PF_CHEST;
			p->a = rngRange(r, x + 1, x + width - 3);
		}
		else if(rngInt(r, 2))
		{
			p->flags |= PF_POT;
			p->a = rngRange(r, x + 1, x + width - 3);
		}
		y += 6;
		x += rngRange(r, -(width - 4), width - 4);
	}
}

static void drawCabin(const Piece *p)
{
	int32_t doorX = (p->flags & PF_DOOR_RIGHT) ? p->x1 : p->x0;

	for(int32_t y = p->y0; y <= p->y1; y++)
	{
		for(int32_t x = p->x0; x <= p->x1; x++)
		{
			bool border = y == p->y0 || y == p->y1 || x == p->x0 || x == p->x1;
			if(!inChunk(x, y)) continue;
			if(border && hash2(x, y, SALT_CABIN) % 10 != 0) setT(x, y, TILE_WOOD);
			else setT(x, y, TILE_NOTHING);
		}
	}
	if(p->flags & PF_PLATFORM)
	{
		for(int32_t x = p->b; x < p->b + p->c; x++) put(x, p->y0, TILE_PLATFORM);
	}
	for(int32_t y = p->y0 + 2; y <= p->y0 + 4; y++) put(doorX, y, TILE_DOOR_C);
	if(p->flags & (PF_CHEST | PF_POT))
	{
		uint8_t l = (p->flags & PF_CHEST) ? TILE_CHEST_L : TILE_POT_L;
		uint8_t rt = (p->flags & PF_CHEST) ? TILE_CHEST_R : TILE_POT_R;
		put(p->a, p->y0 + 3, l); put(p->a + 1, p->y0 + 3, rt);
		put(p->a, p->y0 + 4, l); put(p->a + 1, p->y0 + 4, rt);
	}
//	Cobwebs in the top corners
	if(hash2(p->x0, p->y0, SALT_WEB) & 1) put(p->x0 + 1, p->y0 + 1, TILE_COBWEB);
	if(hash2(p->x1, p->y0, SALT_WEB) & 1) put(p->x1 - 1, p->y0 + 1, TILE_COBWEB);
}

/* Surface house (a small village house). Big enough to pass the NPC housing
   check: an inner area of at least 60 tiles, a chair, a workbench and a torch. */
#define HOUSE_SPACING 6
#define HOUSE_EXTENT 24

static bool houseAt(int rx, Piece *p)
{
	Rng r = rngAt(rx, 0, SALT_HOUSE);
	int32_t hx;
	int inner, biome, mid, s;

	if(rngInt(&r, 100) >= 55) return false;
	hx = (rx * HOUSE_SPACING + rngInt(&r, HOUSE_SPACING - 1)) * CS + rngInt(&r, CS);
	inner = rngRange(&r, 14, 17);
	biome = wgBiome(hx + inner / 2);
	if(biome != BIOME_PLAINS && biome != BIOME_FOREST) return false;
	mid = wgSurfaceY(hx + inner / 2);
	if(mid >= SEA_LEVEL - 1) return false;
	for(int32_t x = hx; x <= hx + inner + 1; x += 3)
	{
		s = wgSurfaceY(x);
		if(iabs(s - mid) > 3) return false;
	}
	s = wgSurfaceY(hx + inner + 1);
	if(iabs(s - mid) > 3) return false;

//	Floor row replaces the surface tile at the middle of the house
	*p = (Piece){hx, mid - 6, hx + inner + 1, mid, 0, 0, 0, PIECE_HOUSE, 0};
	if(rngInt(&r, 2)) p->flags |= PF_DOOR_RIGHT;
	if(rngInt(&r, 2)) p->flags |= PF_CHEST;
	return true;
}

static void drawHouse(const Piece *p)
{
	int32_t fy = p->y1;
	int32_t left = p->x0, right = p->x1;
	bool doorRight = p->flags & PF_DOOR_RIGHT;
	int32_t inX0 = left + 1, inX1 = right - 1;
	int32_t benchX = doorRight ? inX0 + 1 : inX1 - 2;
	int32_t chairX = doorRight ? inX0 + 4 : inX1 - 4;
	int32_t chestX = doorRight ? inX1 - 3 : inX0 + 2;

	for(int32_t x = left; x <= right; x++)
	{
		if(x < chunkX0 || x >= chunkX0 + CS) continue;
//		Foundation down to the ground
		for(int32_t y = fy + 1; y < colAt(x)->surf + 1; y++)
		{
			if(inChunk(x, y) && !isSolid(getT(x, y))) setT(x, y, TILE_DIRT);
		}
//		Clear a little headroom above the roof
		for(int32_t y = p->y0 - 3; y < p->y0; y++)
		{
			if(inChunk(x, y) && isSolid(getT(x, y))) setT(x, y, TILE_NOTHING);
		}
		for(int32_t y = p->y0; y <= fy; y++)
		{
			if(!inChunk(x, y)) continue;
			if(y == p->y0 || y == fy || x == left || x == right) setT(x, y, TILE_WOOD);
			else setT(x, y, TILE_NOTHING);
		}
	}
//	Door in a side wall
	for(int32_t y = fy - 3; y <= fy - 1; y++) put(doorRight ? right : left, y, TILE_DOOR_C);

//	Furniture
	put(benchX, fy - 1, TILE_WBENCH_L);
	put(benchX + 1, fy - 1, TILE_WBENCH_R);
	put(chairX, fy - 2, doorRight ? TILE_CHAIR_L : TILE_CHAIR_R);
	put(chairX, fy - 1, doorRight ? TILE_CHAIR_L : TILE_CHAIR_R);
	put((inX0 + inX1) / 2, fy - 4, TILE_TORCH);
	if(p->flags & PF_CHEST)
	{
		put(chestX, fy - 2, TILE_CHEST_L); put(chestX + 1, fy - 2, TILE_CHEST_R);
		put(chestX, fy - 1, TILE_CHEST_L); put(chestX + 1, fy - 1, TILE_CHEST_R);
	}
}

#define MINE_SPACING 8
#define MINE_SEPARATION 2
#define MINE_EXTENT 112
#define CABIN_SPACING 4
#define CABIN_SEPARATION 1
#define CABIN_EXTENT 48

static void drawStructures(void)
{
	int32_t x1 = chunkX0 + CS - 1, y1 = chunkY0 + CS - 1;
	int32_t regW, sx, sy;
	Rng r;
	Piece house;

//	Buried cabins
	regW = CABIN_SPACING * CS;
	for(int ry = floorDiv(chunkY0 - CABIN_EXTENT, regW); ry <= floorDiv(y1 + CABIN_EXTENT, regW); ry++)
	{
		for(int rx = floorDiv(chunkX0 - CABIN_EXTENT, regW); rx <= floorDiv(x1 + CABIN_EXTENT, regW); rx++)
		{
			structureStart(rx, ry, CABIN_SPACING, CABIN_SEPARATION, SALT_CABIN, &sx, &sy, &r);
			if(rngInt(&r, 100) >= 50) continue;
			if(iabs(sx - chunkX0) > CABIN_EXTENT + CS || iabs(sy - chunkY0) > CABIN_EXTENT + CS) continue;
			if(sy < wgSurfaceY(sx) + 25) continue;
			buildCabin(sx, sy, &r);
			for(int i = 0; i < numPieces; i++)
			{
				if(piecesIntersect(&pieces[i], chunkX0, chunkY0, x1, y1)) drawCabin(&pieces[i]);
			}
		}
	}

//	Mineshafts
	regW = MINE_SPACING * CS;
	for(int ry = floorDiv(chunkY0 - MINE_EXTENT, regW); ry <= floorDiv(y1 + MINE_EXTENT, regW); ry++)
	{
		for(int rx = floorDiv(chunkX0 - MINE_EXTENT, regW); rx <= floorDiv(x1 + MINE_EXTENT, regW); rx++)
		{
			structureStart(rx, ry, MINE_SPACING, MINE_SEPARATION, SALT_MINE, &sx, &sy, &r);
			if(rngInt(&r, 100) >= 45) continue;
			if(iabs(sx - chunkX0) > MINE_EXTENT + CS || iabs(sy - chunkY0) > MINE_EXTENT + CS) continue;
			if(sy < wgSurfaceY(sx) + 35) continue;
			buildMineshaft(sx, sy, &r);
			for(int i = 0; i < numPieces; i++)
			{
				if(piecesIntersect(&pieces[i], chunkX0, chunkY0, x1, y1)) drawMinePiece(&pieces[i]);
			}
		}
	}

//	Surface houses
	regW = HOUSE_SPACING * CS;
	numHouseSpans = 0;
	for(int rx = floorDiv(chunkX0 - HOUSE_EXTENT, regW); rx <= floorDiv(x1 + HOUSE_EXTENT, regW); rx++)
	{
		if(!houseAt(rx, &house)) continue;
		if(house.x1 + 2 < chunkX0 || house.x0 - 2 > x1) continue;
//		Don't build into a lake
		bool wet = false;
		for(int i = 0; i < numLakeSpans; i++)
		{
			if(lakeSpans[i].x0 <= house.x1 && lakeSpans[i].x1 >= house.x0) wet = true;
		}
		if(wet) continue;
		if(numHouseSpans < MAX_SPANS) houseSpans[numHouseSpans++] = (Span){house.x0 - 2, house.x1 + 2};
		if(house.y1 + 8 < chunkY0 || house.y0 - 4 > y1) continue;
		drawHouse(&house);
	}
}

//---
// Ores
//---

typedef struct {
	uint8_t tile;
	uint8_t attempts;		// Per chunk
	int16_t minY, maxY;	// Absolute rows
	uint8_t minSize, maxSize;
	bool triangle;			// Triangular height distribution, peak in the middle
} OreFeature;

static const OreFeature ores[] = {
	{TILE_DIRT,			5,	4,		400,	10, 22, false},
	{TILE_CLAY,			2,	-20,	60,		6,	12, false},
	{TILE_COPPER_ORE,	5,	-10,	120,	4,	9,	true},
	{TILE_TIN_ORE,		4,	10,		180,	4,	9,	true},
	{TILE_IRON_ORE,		4,	30,		32000,	4,	10, false},
};

static inline bool oreReplaces(uint8_t ore, uint8_t t)
{
	if(ore == TILE_CLAY) return t == TILE_STONE || t == TILE_DIRT || t == TILE_SAND;
	return t == TILE_STONE;
}

static void drawOres(int cx, int cy)
{
	static const int8_t steps[8][2] = {
		{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
	};

	for(unsigned int k = 0; k < sizeof(ores) / sizeof(ores[0]); k++)
	{
		const OreFeature *ore = &ores[k];
		for(int scy = cy - 1; scy <= cy + 1; scy++)
		{
			if((scy + 1) * CS + ore->maxSize < ore->minY || scy * CS - ore->maxSize > ore->maxY) continue;
			for(int scx = cx - 1; scx <= cx + 1; scx++)
			{
				Rng r = rngAt(scx, scy, SALT_ORE + k * 0x1000);
				for(int a = 0; a < ore->attempts; a++)
				{
					int32_t x = scx * CS + rngInt(&r, CS);
					int32_t y = scy * CS + rngInt(&r, CS);
					int size = rngRange(&r, ore->minSize, ore->maxSize);
					uint32_t roll = rngNext(&r);

					if(y < ore->minY || y > ore->maxY) continue;
					if(ore->triangle)
					{
						int half = (ore->maxY - ore->minY) / 2;
						int dist = iabs(y - (ore->minY + half));
						if((int)(roll % (uint32_t)half) < dist) continue;
					}
					for(int i = 0; i < size; i++)
					{
						const int8_t *st;
						if(inChunk(x, y) && oreReplaces(ore->tile, getT(x, y))) setT(x, y, ore->tile);
						st = steps[rngInt(&r, 8)];
						x += st[0];
						y += st[1];
					}
				}
			}
		}
	}
}

//---
// Decoration and vegetation
//---

static bool inSpans(const Span *spans, int n, int32_t gx)
{
	for(int i = 0; i < n; i++) if(gx >= spans[i].x0 && gx <= spans[i].x1) return true;
	return false;
}

static bool fits2x2(int32_t x, int32_t y)
{
	if(!inChunk(x, y) || !inChunk(x + 1, y + 2)) return false;
	for(int dy = 0; dy < 2; dy++)
		for(int dx = 0; dx < 2; dx++)
			if(getT(x + dx, y + dy) != TILE_NOTHING) return false;
	return isSolid(getT(x, y + 2)) && isSolid(getT(x + 1, y + 2));
}

static void put2x2(int32_t x, int32_t y, uint8_t l, uint8_t r)
{
	setT(x, y, l); setT(x + 1, y, r);
	setT(x, y + 1, l); setT(x + 1, y + 1, r);
}

static void drawUndergroundDecoration(void)
{
//	Underground jungle: mud exposed to air grows jungle grass
	for(int y = 0; y < CS; y++)
	{
		for(int x = 0; x < CS; x++)
		{
			int32_t gx = chunkX0 + x, gy = chunkY0 + y;
			if(getT(gx, gy) != TILE_MUD) continue;
			if((x > 0 && getT(gx - 1, gy) == TILE_NOTHING)
			|| (x < CS - 1 && getT(gx + 1, gy) == TILE_NOTHING)
			|| (y > 0 && getT(gx, gy - 1) == TILE_NOTHING)
			|| (y < CS - 1 && getT(gx, gy + 1) == TILE_NOTHING))
				setT(gx, gy, TILE_GRASS_JUNGLE);
		}
	}

	for(int y = 0; y < CS; y++)
	{
		for(int x = 0; x < CS; x++)
		{
			int32_t gx = chunkX0 + x, gy = chunkY0 + y;
			int depth = gy - colAt(gx)->surf;
			uint32_t h;

			if(depth < 8 || getT(gx, gy) != TILE_NOTHING) continue;
			h = hash2(gx, gy, SALT_GEM);

//			Pots and life crystals on cave floors
			if(x < CS - 1 && y < CS - 2)
			{
				if(depth > 40 && h % 400 == 0 && fits2x2(gx, gy))
				{
					put2x2(gx, gy, TILE_CRYST_L, TILE_CRYST_R);
					continue;
				}
				if(h % 40 == 1 && fits2x2(gx, gy))
				{
					put2x2(gx, gy, TILE_POT_L, TILE_POT_R);
					continue;
				}
			}

//			Gems deep down, resting on stone
			if(gy > 60 && (h >> 12) % 13 == 0)
			{
				if((y < CS - 1 && getT(gx, gy + 1) == TILE_STONE)
				|| (x > 0 && getT(gx - 1, gy) == TILE_STONE)
				|| (x < CS - 1 && getT(gx + 1, gy) == TILE_STONE))
				{
					setT(gx, gy, TILE_AMETHYST + (h >> 20) % 6);
					continue;
				}
			}

//			Cobwebs under cave ceilings
			if(depth > 14 && y > 0 && isSolid(getT(gx, gy - 1)) && (h >> 8) % 14 == 0)
			{
				setT(gx, gy, TILE_COBWEB);
				if(y < CS - 1 && getT(gx, gy + 1) == TILE_NOTHING && (h & 1)) setT(gx, gy + 1, TILE_COBWEB);
				continue;
			}

//			Jungle plants in underground jungle
			if(y < CS - 1 && getT(gx, gy + 1) == TILE_GRASS_JUNGLE && (h >> 4) % 3 == 0)
			{
				setT(gx, gy, TILE_PLANT);
			}
		}
	}
}

/* Pure: does column gx have a grassy, dry, uncarved surface tile? */
static bool grassyGround(const Column *col)
{
	return (col->flags & COL_GRASS) && (col->flags & COL_DRY);
}

static int treeHeight(int32_t gx, const Column *col)
{
	int32_t cell = floorDiv(gx, 5);
	uint32_t h = hash1(cell, SALT_TREE);
	int density;

	if(gx != cell * 5 + 1 + (int32_t)(h % 3)) return 0;
	switch(col->biome)
	{
		case BIOME_FOREST: density = 75; break;
		case BIOME_JUNGLE: density = 85; break;
		case BIOME_SWAMP: density = 35; break;
		case BIOME_PLAINS: density = 10; break;
		case BIOME_MOUNTAINS: density = 15; break;
		default: return 0;
	}
	if((int)((h >> 8) % 100) >= density) return 0;
	if(!grassyGround(col)) return 0;
	if(inSpans(lakeSpans, numLakeSpans, gx) || inSpans(houseSpans, numHouseSpans, gx)) return 0;

	switch(col->biome)
	{
		case BIOME_JUNGLE: return 10 + (h >> 16) % 9;
		case BIOME_FOREST: return 5 + (h >> 16) % 8;
		default: return 4 + (h >> 16) % 5;
	}
}

static void drawVegetation(void)
{
	int32_t x1 = chunkX0 + CS - 1, y1 = chunkY0 + CS - 1;

//	Trees: trunk, a treetop tile (drawn as a big sprite) and roots
	for(int32_t gx = chunkX0 - 1; gx <= x1 + 1; gx++)
	{
		Column *col = colAt(gx);
		int height = treeHeight(gx, col);
		int32_t s = col->surf;
		uint32_t h;

		if(!height) continue;
		if(s - height - 1 > y1 || s - 1 < chunkY0) continue;
		h = hash1(gx, SALT_TREE + 1);
		for(int i = 1; i <= height; i++)
		{
			if(inChunk(gx, s - i) && getT(gx, s - i) == TILE_NOTHING) setT(gx, s - i, TILE_TRUNK);
		}
		if(inChunk(gx, s - height - 1) && getT(gx, s - height - 1) == TILE_NOTHING) setT(gx, s - height - 1, TILE_LEAVES);
		if((h & 3) != 0 && colAt(gx - 1)->surf == s && grassyGround(colAt(gx - 1))
		&& inChunk(gx - 1, s - 1) && getT(gx - 1, s - 1) == TILE_NOTHING) setT(gx - 1, s - 1, TILE_ROOT_L);
		if((h & 12) != 0 && colAt(gx + 1)->surf == s && grassyGround(colAt(gx + 1))
		&& inChunk(gx + 1, s - 1) && getT(gx + 1, s - 1) == TILE_NOTHING) setT(gx + 1, s - 1, TILE_ROOT_R);
	}

//	Cacti
	for(int32_t gx = chunkX0 - 1; gx <= x1 + 1; gx++)
	{
		Column *col = colAt(gx);
		int32_t cell = floorDiv(gx, 7);
		uint32_t h = hash1(cell, SALT_CACTUS);
		int32_t s = col->surf;
		int height;
		int lastBranch = 0;

		if(col->biome != BIOME_DESERT || !(col->flags & COL_DRY)) continue;
		if(gx != cell * 7 + 2 + (int32_t)(h % 3) || (h >> 8) % 100 >= 45) continue;
		if(inSpans(lakeSpans, numLakeSpans, gx)) continue;
		height = 3 + (h >> 16) % 5;
		for(int i = 1; i <= height; i++)
		{
			int32_t gy = s - i;
			uint32_t b = hash2(gx, i, SALT_CACTUS);
			put(gx, gy, TILE_CACTUS);
			if(i > 1 && i < height)
			{
				if(b % 4 == 0 && lastBranch != -1) { put(gx - 1, gy, TILE_CACTUS_BRANCH); lastBranch = -1; }
				else if(b % 4 == 1 && lastBranch != 1) { put(gx + 1, gy, TILE_CACTUS_BRANCH); lastBranch = 1; }
				else lastBranch = 0;
			}
		}
	}

//	Plants and mushrooms on grass
	for(int32_t gx = chunkX0; gx <= x1; gx++)
	{
		Column *col = colAt(gx);
		int32_t gy = col->surf - 1;
		uint32_t h;

		if(!grassyGround(col) || !inChunk(gx, gy) || getT(gx, gy) != TILE_NOTHING) continue;
		if(inSpans(lakeSpans, numLakeSpans, gx) || inSpans(houseSpans, numHouseSpans, gx)) continue;
		h = hash1(gx, SALT_PLANT);
		if(h % 4 == 0) continue;
		setT(gx, gy, (h >> 8) % 8 == 0 ? TILE_MUSHROOM : TILE_PLANT);
	}

//	Vines hanging from grass ceilings (overhangs, cave mouths, jungle caves)
	for(int y = 0; y < CS - 1; y++)
	{
		for(int x = 0; x < CS; x++)
		{
			int32_t gx = chunkX0 + x, gy = chunkY0 + y;
			uint8_t t = getT(gx, gy);
			uint32_t h;
			int len;

			if((t != TILE_GRASS && t != TILE_GRASS_JUNGLE) || getT(gx, gy + 1) != TILE_NOTHING) continue;
			h = hash2(gx, gy, SALT_VINE);
			if(h % 3 == 0 || (t == TILE_GRASS && h % 3 == 1)) continue;
			len = 2 + (h >> 8) % 8;
			for(int i = 1; i <= len && y + i < CS && getT(gx, gy + i) == TILE_NOTHING; i++) setT(gx, gy + i, TILE_VINE);
		}
	}
}

//---
// Chunk generation
//---

static void buildColumns(void)
{
	Climate cl;
	int surf, biome;

	for(int i = 0; i < NUM_COLS; i++)
	{
		int32_t gx = chunkX0 + i - COL_MARGIN;
		Column *col = &cols[i];

		evalColumn(gx, &surf, &biome, &cl);
		col->surf = surf;
		col->biome = biome;
		col->flags = surf <= SEA_LEVEL ? COL_DRY : 0;
		col->jungle = surf <= SEA_LEVEL ? clampQ11(imin(cl.t - 250, cl.h - 450) * 2) : -2048;
		col->dirtDepth = 3 + (vnoise1(gx, 3, SALT_DIRT) >> 10);
	}

	for(int i = 0; i < NUM_COLS; i++)
	{
		Column *col = &cols[i];
		int protect = INT16_MIN;
		int steep;

//		Near water, caves must stay a few tiles below the water's floor
		for(int k = -PROTECT_RADIUS; k <= PROTECT_RADIUS; k++)
		{
			int j = i + k;
			if(j < 0 || j >= NUM_COLS) continue;
			if(cols[j].surf > SEA_LEVEL) protect = imax(protect, cols[j].surf + 8);
		}
		col->protectY = protect;

		steep = (i > 0 && i < NUM_COLS - 1) ? iabs(cols[i + 1].surf - cols[i - 1].surf) : 0;
		if(col->biome == BIOME_MOUNTAINS && (steep >= 4 || col->surf < SEA_LEVEL - 48)) col->flags |= COL_STONE_TOP;

		if((col->flags & COL_DRY) && !(col->flags & COL_STONE_TOP))
		{
			switch(col->biome)
			{
				case BIOME_PLAINS: case BIOME_FOREST: case BIOME_JUNGLE:
				case BIOME_SWAMP: case BIOME_MOUNTAINS:
//					A spaghetti cave entrance may have removed the surface tile
					if(col->surf >= col->protectY && noiseCaveAt(chunkX0 + i - COL_MARGIN, col->surf, 0)) break;
					col->flags |= COL_GRASS;
					break;
				default:
					break;
			}
		}
	}
}

static uint8_t surfaceTile(const Column *col, int32_t gx, int32_t gy)
{
	int d = gy - col->surf;

	if(d < 0) return gy >= SEA_LEVEL ? TILE_WATER : TILE_NOTHING;

	if(!(col->flags & COL_DRY))
	{
//		Sea, river and lake beds
		if(d < 4)
		{
			if(col->biome == BIOME_SWAMP || col->biome == BIOME_JUNGLE) return TILE_MUD;
			return (hash2(gx >> 2, gy, SALT_FLOOR) & 7) == 0 ? TILE_CLAY : TILE_SAND;
		}
		if(d < 7) return TILE_DIRT;
		return TILE_STONE;
	}

	switch(col->biome)
	{
		case BIOME_OCEAN:
		case BIOME_BEACH:
			if(d < 5) return TILE_SAND;
			break;

		case BIOME_DESERT:
			if(d < 5 + col->dirtDepth / 2) return TILE_SAND;
			if(d < 8 + col->dirtDepth / 2) return TILE_CLAY;
			break;

		case BIOME_JUNGLE:
			if(d == 0) return TILE_GRASS_JUNGLE;
			if(d < 10) return TILE_MUD;
			break;

		case BIOME_SWAMP:
			if(d == 0) return TILE_GRASS;
			if(d < 5) return TILE_MUD;
			if(d < 7) return TILE_DIRT;
			break;

		case BIOME_MOUNTAINS:
			if(col->flags & COL_STONE_TOP) break;
			if(d == 0) return TILE_GRASS;
			if(d < 3) return TILE_DIRT;
			break;

		default:
			if(d == 0) return TILE_GRASS;
			if(d < col->dirtDepth) return TILE_DIRT;
			break;
	}
	return TILE_STONE;
}

void wgGenerateChunk(int cx, int cy, uint8_t *out, int stride)
{
	int32_t minSurf = INT32_MAX;
	bool underground;

	chunkOut = out;
	chunkStride = stride;
	chunkX0 = cx * CS;
	chunkY0 = cy * CS;
	numLakeSpans = numHouseSpans = 0;

	buildColumns();
	for(int i = 0; i < NUM_COLS; i++) minSurf = imin(minSurf, cols[i].surf);

//	1-3. Terrain, water and surface rules
	for(int y = 0; y < CS; y++)
	{
		for(int x = 0; x < CS; x++)
		{
			int32_t gx = chunkX0 + x;
			setT(gx, chunkY0 + y, surfaceTile(colAt(gx), gx, chunkY0 + y));
		}
	}

//	Sky: nothing reaches this high
	if(chunkY0 + CS - 1 < minSurf - 26) return;

	underground = chunkY0 + CS - 1 >= minSurf;

//	4. Noise caves and underground jungle, using a 4x4 grid like Minecraft's
//	noise cells
	if(underground)
	{
		for(int j = 0; j < GRID; j++)
		{
			for(int i = 0; i < GRID; i++)
			{
				int16_t f[NUM_FIELDS];
				caveFields(chunkX0 + 4 * i, chunkY0 + 4 * j, f);
				for(int k = 0; k < NUM_FIELDS; k++) grid[k][j][i] = f[k];
			}
		}

		for(int y = 0; y < CS; y++)
		{
			for(int x = 0; x < CS; x++)
			{
				int32_t gx = chunkX0 + x, gy = chunkY0 + y;
				Column *col = colAt(gx);
				int d = gy - col->surf;
				int i = x >> 2, j = y >> 2, fx = x & 3, fy = y & 3;
				int f[NUM_FIELDS];
				uint8_t t;

				if(d < 0) continue;
				t = getT(gx, gy);
				for(int k = 0; k < NUM_FIELDS; k++)
					f[k] = lerpCell(grid[k][j][i], grid[k][j][i + 1], grid[k][j + 1][i], grid[k][j + 1][i + 1], fx, fy);

				if(t == TILE_STONE && d >= 6 && d < 120)
				{
					int jungle = col->jungle + (f[5] >> 1) - imax(0, d - 90) * 60;
					if(jungle > 0) setT(gx, gy, t = TILE_MUD);
				}

				if(gy >= col->protectY && d >= sandRoof(col) && caveTest(f, d)) setT(gx, gy, TILE_NOTHING);
			}
		}

		carveWorms(cx, cy);
	}

//	5. Lakes
	drawLakes(cx);

//	6. Structures
	drawStructures();

	if(underground)
	{
//		7. Ores
		drawOres(cx, cy);
//		8. Underground decoration
		drawUndergroundDecoration();
	}

//	9. Vegetation
	drawVegetation();
}

int wgFindSpawnX(void)
{
	for(int i = 0; i < 8192; i++)
	{
		int gx = (i & 1) ? -(i >> 1) * 4 : (i >> 1) * 4;
		Climate cl;
		int surf, biome;

		evalColumn(gx, &surf, &biome, &cl);
		if(biome != BIOME_PLAINS && biome != BIOME_FOREST) continue;
		if(surf >= SEA_LEVEL - 2) continue;
//		Stay away from water and cave mouths
		bool ok = true;
		for(int k = -PROTECT_RADIUS; k <= PROTECT_RADIUS && ok; k++)
		{
			if(wgSurfaceY(gx + k) > SEA_LEVEL - 1) ok = false;
		}
		if(!ok || noiseCaveAt(gx, surf, 0)) continue;
		return gx;
	}
	return 0;
}
