#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <gint/defs/util.h>

#include "generate.h"
#include "world.h"
#include "chest.h"

#define NUM_WORLD_GEN_PARTS 29

GYRAM Coords checkCoords[CHECK_BUFFER_SIZE];

static float progress = 0;

int updateProgress()
{
	progress += 100.0 / NUM_WORLD_GEN_PARTS;

	return (int)(progress);
}

float interpolate(float a, float b, float x){
	float f = (1.0 - cosf(x * PI)) * 0.5;
    return a * (1.0 - f) + b * f;
}

/* floorDiv
Integer division rounded towards negative infinity, needed to sample the
global terrain noise on the left side of the starting chunk. */
static int floorDiv(int value, int divisor)
{
	int quotient = value / divisor;
	int remainder = value % divisor;

	if(remainder != 0 && ((remainder < 0) != (divisor < 0))) quotient--;
	return quotient;
}

static uint32_t hashU32(uint32_t value)
{
	value ^= value >> 16;
	value *= 0x7feb352dU;
	value ^= value >> 15;
	value *= 0x846ca68bU;
	value ^= value >> 16;
	return value;
}

/* valueNoise
One-dimensional value noise sampled in global tile coordinates, so the same
position evaluates to the same height in every chunk. This is what keeps the
surface continuous when a new 160x75 chunk is generated next to an old one. */
static float valueNoise(int globalX, int wavelength, unsigned int salt)
{
	int cell = floorDiv(globalX, wavelength);
	float t = (float)(globalX - cell * wavelength) / (float)wavelength;
	uint32_t base = save.worldSeed ^ (salt * 0x9e3779b9U);
	float a = (float)(hashU32((uint32_t)cell ^ base) & 0xffff) / 65535.0f;
	float b = (float)(hashU32((uint32_t)(cell + 1) ^ base) & 0xffff) / 65535.0f;

	return interpolate(a, b, t);
}

static uint32_t chunkSeed(int chunkX, int chunkY)
{
	return hashU32(save.worldSeed
		^ ((uint32_t)chunkX * 0x9e3779b1U)
		^ ((uint32_t)chunkY * 0x85ebca6bU));
}

float randFloat()
{
	return (float)rand() / 0x7fffffff;
}

int randRange(int low, int high)
{
	if(high <= low) return low;
	return (rand() % (high - low)) + low;
}

int poisson(double lambda)
{
	int k = 0;
	float p = 1;
	double L = pow(E, -lambda);
	while(p > L)
	{
		k++;
		p *= randFloat();
	}
	return k - 1;
}

void perlin(int amplitude, int wavelength, int baseY, enum Tiles tile, int iterations, unsigned int salt)
{
	int yPositions[game.WORLD_WIDTH];
	int base = baseY - save.chunkY * game.WORLD_HEIGHT;
	int perlinY;

	for(int x = 0; x < game.WORLD_WIDTH; x++) yPositions[x] = base;

	for(int i = 0; i < iterations; i++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++)
		{
			int globalX = save.chunkX * game.WORLD_WIDTH + x;
			perlinY = valueNoise(globalX, wavelength, salt + i * 0x85ebca6bU) * amplitude;
			yPositions[x] += perlinY;
		}
		wavelength >>= 1;
		amplitude >>= 1;
		if(wavelength == 0 || amplitude == 0) break;
	}

	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = max(0, yPositions[x]); y < game.WORLD_HEIGHT; y++)
		{
			setTile(x, y, tile);
		}
	}
}

void clump(int x, int y, int num, enum Tiles tile, bool maskEmpty, float skewHorizontal, float skewVertical)
{
	int end = 1;
	int selected;
	Coords selectedTile;
	int deltas[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	Tile tileCheck;
	int checkX, checkY;

	if(maskEmpty && getTile(x, y).id == TILE_NOTHING) return;

	checkCoords[0] = (Coords){x, y};

	while(num > 0)
	{
		if(end == 0) return;
		selected = (unsigned int)rand() % end;
		selectedTile = checkCoords[selected];
		checkCoords[selected] = checkCoords[end - 1];
		end--;
		setTile(selectedTile.x, selectedTile.y, tile);
		num--;
		for(int delta = 0; delta < 4; delta++)
		{
			checkX = selectedTile.x + deltas[delta][0];
			checkY = selectedTile.y + deltas[delta][1];
			if(skewHorizontal > 0)
			{
				if(deltas[delta][1] != 0 && randFloat() < skewHorizontal) continue;
			}
			if(skewVertical > 0)
			{
				if(deltas[delta][0] != 0 && randFloat() < skewVertical) continue;
			}
			if(checkX < 0 || checkX >= game.WORLD_WIDTH || checkY < 0 || checkY >= game.WORLD_HEIGHT) continue;
			tileCheck = getTile(checkX, checkY);
			if(tileCheck.id == tile || (maskEmpty && tileCheck.id == TILE_NOTHING)) continue;
			if(end >= CHECK_BUFFER_SIZE) continue;
			checkCoords[end++] = (Coords){checkX, checkY};
		}
	}
}

void box(int x, int y, int width, int height, enum Tiles tile, float coverage, enum Tiles swap)
{
	for(int dX = 0; dX < width; dX++)
	{
		for(int dY = 0; dY < height; dY++)
		{
			if(dY == 0 || dY == height - 1 || dX == 0 || dX == width - 1)
			{
				if(randFloat() < coverage)
				{
					setTile(x + dX, y + dY, tile);
				}
				else{
					setTile(x + dX, y + dY, swap);
				}
			}
			else
			{
				setTile(x + dX, y + dY, TILE_NOTHING);
			}
		}
	}
}

void parabola(int x, int y, int width, int depth, enum Tiles material)
{
	float multiplier = (float)-depth / pow((float)width / 2, 2);
	for(int dX = 0; dX < width; dX++)
	{
		if(x + dX < 0) continue;
		else if(x + dX >= game.WORLD_WIDTH) return;
		for(int dY = 0; dY < y; dY++)
		{
			if(getTile(x + dX, dY).id != TILE_NOTHING) setTile(x + dX, dY, TILE_NOTHING);
		}
		for(int dY = 0; dY < multiplier * pow(dX - width / 2, 2) + depth; dY++)
		{
			setTile(x + dX, y + dY, material);
		}
	}
}

/* sealMaterial
Picks a natural-looking material for a water-sealing plug, based on what the plug
sits under. Never returns sand, which would fall away and re-open the leak.

x, y: Coordinates of the tile the plug goes directly below.

Returns the tile to place.
*/
static enum Tiles sealMaterial(int x, int y)
{
	switch(getTile(x, y).id)
	{
		case TILE_DIRT:
		case TILE_GRASS:
		case TILE_GRASS_JUNGLE:
		case TILE_MUD:
		case TILE_CLAY:
		case TILE_SAND:
			return TILE_DIRT;

		case TILE_STONE:
		case TILE_IRON_ORE:
		case TILE_COPPER_ORE:
		case TILE_TIN_ORE:
		case TILE_AMETHYST:
		case TILE_DIAMOND:
		case TILE_EMERALD:
		case TILE_RUBY:
		case TILE_SAPPHIRE:
		case TILE_TOPAZ:
			return TILE_STONE;

		default:
			return (rand() % 3) ? TILE_DIRT : TILE_STONE;
	}
}

/*
Keeping generation step names the same as Terraria just to make it easier to follow its world gen process
*/
void generateWorld()
{
	int x, y;
	Tile tile;
	int height, copseHeight;
	int tunnelYPositions[10];
	int num, width, tempX, tempY, tries;
	bool placedChest;
	Item check;
	int stage;
	int left;
	int deltaX;
	float mul;
	int depth, leftY, rightY;
	bool surfaceChunk = save.chunkY == 0;
	bool undergroundChunk = save.chunkY >= 0;

	progress = 0;
	srand(chunkSeed(save.chunkX, save.chunkY));

	middleText("Reset", updateProgress());
	for(int y = 0; y < game.WORLD_HEIGHT; y++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++) setTile(x, y, TILE_NOTHING);
	}

	middleText("Terrain", updateProgress());
//	Dirt
	perlin(6, 30, game.WORLD_HEIGHT / 5, TILE_DIRT, 3, 0x1b873593U);
//	Stone
	perlin(6, 20, game.WORLD_HEIGHT / 2.8, TILE_STONE, 1, 0xcc9e2d51U);

//	Tunnels
	middleText("Tunnels", updateProgress());
	if(surfaceChunk)
	{
		x = 20;
		while(x < game.WORLD_WIDTH - 20)
		{
			if(randFloat() < 0.01)
			{
				for(int dX = 0; dX < 50; dX += 5)
				{
					y = 0;
					while(getTile(x + dX, y + 8).id != TILE_DIRT && y < game.WORLD_HEIGHT) y++;
					tunnelYPositions[dX / 5] = y;
				}
				for(int dX = 0; dX < 50; dX += 5)
				{
					clump(x + dX, tunnelYPositions[dX / 5], 20, TILE_DIRT, false, 0.5, 0);
				}
				x += 40;
			}
			else x++;
		}
	}

//	Sand
	middleText("Sand", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < 40 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.5, game.WORLD_HEIGHT / 2);
			clump(x, y, poisson(40), TILE_SAND, true, 0, 0);
		}
		for(int i = 0; i < max(2, poisson(3)); i++)
		{
			width = max(1, poisson(60) * game.WORLDGEN_MULTIPLIER);
			mul = -60 / width;
			x = randRange(0, game.WORLD_WIDTH - width);
			for(int dX = 0; dX < width; dX++)
			{
				left = min(20, mul * abs(dX - width / 2) + 30);
				y = 0;
				while(left > 0)
				{
					if(getTile(x + dX, y).id != TILE_NOTHING)
					{
						setTile(x + dX, y, TILE_SAND);
						left--;
					}
					y++;
				}
			}
		}
	}

//	Rocks in dirt
	middleText("Rocks In Dirt", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = rand() % (int)(game.WORLD_HEIGHT / 2.8);
			if(getTile(x, y).id == TILE_DIRT)
			{
				clump(x, y, poisson(10), TILE_STONE, true, 0, 0);
			}
		}
	}

//	Dirt in rocks
	middleText("Dirt In Rocks", updateProgress());
	if(undergroundChunk)
	{
		for(int i = 0; i < 3000 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
			if(getTile(x, y).id == TILE_STONE)
			{
				clump(x, y, poisson(10), TILE_DIRT, true, 0, 0);
			}
		}
	}

//	Clay
	middleText("Clay", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = rand() % (int)(game.WORLD_HEIGHT / 4.2);
			if(getTile(x, y).id == TILE_DIRT)
			{
				clump(x, y, poisson(10), TILE_CLAY, true, 0, 0);
			}
		}
	}

//	Small holes
	middleText("Small Holes", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < 250 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
			clump(x, y, poisson(50), TILE_WATER, false, 0, 0);
		}
		for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
			clump(x, y, poisson(50), TILE_NOTHING, true, 0, 0);
		}
	}

//	Caves
	middleText("Caves", updateProgress());
	if(undergroundChunk)
	{
		for(int i = 0; i < 150 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
			clump(x, y, poisson(200), TILE_NOTHING, true, 0, 0);
		}
	}

//	Surface Caves
	middleText("Surface Caves", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < 150 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 4, game.WORLD_HEIGHT / 2.8);
			clump(x, y, poisson(125), TILE_NOTHING, true, 0, 0);
		}
	}

//	Grass
	middleText("Grass", updateProgress());
	if(surfaceChunk)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++)
		{
			for(int y = 0; y < game.WORLD_HEIGHT / 2.8; y++)
			{
				tile = getTile(x, y);
				if(tile.id == TILE_DIRT)
				{
					setTile(x, y, TILE_GRASS);
					if(x == 0 || x == game.WORLD_WIDTH - 1 || y == game.WORLD_HEIGHT) break;
					if(getTile(x - 1, y).id == TILE_NOTHING || getTile(x + 1, y).id == TILE_NOTHING)
					{
						setTile(x, y + 1, TILE_GRASS);
					}
					break;
				}
				else if(tile.id != TILE_NOTHING) break;
			}
		}

		for(int x = 0; x < game.WORLD_WIDTH; x++)
		{
			for(int y = 0; y < game.WORLD_HEIGHT / 2.8; y++)
			{
				if(getTile(x, y).id == TILE_DIRT && findState(x, y) != 0xf)
				{
					setTile(x, y, TILE_GRASS);
				}
			}
		}
	}

//	Jungle
	middleText("Jungle", updateProgress());
	if(surfaceChunk)
	{
		width = poisson(150) * game.WORLDGEN_MULTIPLIER;
		x = (rand() % 2) ? game.WORLD_WIDTH / 5 : game.WORLD_WIDTH - game.WORLD_WIDTH / 5 - width;
		for(int y = 0; y < game.WORLD_HEIGHT; y++)
		{
			deltaX = 0;
			while(deltaX < width && getTile(x + deltaX, y).id == TILE_NOTHING) deltaX++;
			while(deltaX < width)
			{
				tile = getTile(x + deltaX, y);
				if(deltaX < 20 || width - deltaX < 20)
				{
					num = (deltaX < 20) ? (20 - deltaX) : (20 - (20 - width - deltaX));
					if(randRange(0, num) > 5)
					{
						deltaX++;
						continue;
					}
				}
				if(tile.id == TILE_DIRT || tile.id == TILE_STONE || tile.id == TILE_SAND) setTile(x + deltaX, y, TILE_MUD);
				else if(tile.id == TILE_GRASS) setTile(x + deltaX, y, TILE_GRASS_JUNGLE);
				deltaX++;
			}
		}
		for(int dX = 0; dX < width; dX++)
		{
			for(int y = 0; y < game.WORLD_HEIGHT; y++)
			{
				tile = getTile(x + dX, y);
				if(tile.id == TILE_MUD && findState(x + dX, y) != 0xf) setTile(x + dX, y, TILE_GRASS_JUNGLE);
			}
		}
	}

//	Shinies
	middleText("Shinies", updateProgress());
	if(undergroundChunk)
	{
		for(int i = 0; i < 500 * game.WORLDGEN_MULTIPLIER; i++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = rand() % game.WORLD_HEIGHT;
			if(getTile(x, y).id != TILE_SAND)
			{
				clump(x, y, poisson(6), TILE_IRON_ORE, true, 0, 0);
			}
		}

		if(rand() % 2)
		{
			for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
			{
				x = rand() % game.WORLD_WIDTH;
				y = rand() % game.WORLD_HEIGHT;
				if(getTile(x, y).id != TILE_SAND)
				{
					clump(x, y, poisson(8), TILE_COPPER_ORE, true, 0, 0);
				}
			}
		}
		else
		{
			for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
			{
				x = rand() % game.WORLD_WIDTH;
				y = rand() % game.WORLD_HEIGHT;
				if(getTile(x, y).id != TILE_SAND)
				{
					clump(x, y, poisson(10), TILE_TIN_ORE, true, 0, 0);
				}
			}
		}
	}

//	Lakes
	middleText("Lakes", updateProgress());
	if(surfaceChunk)
	{
		for(int i = 0; i < max(2, poisson(3)); i++)
		{
			width = max(8, min(game.WORLD_WIDTH / 3, poisson(15)));
			x = randRange(10, game.WORLD_WIDTH - 10 - width);
			depth = max(5, poisson(10));
			leftY = game.WORLD_HEIGHT / 5;
			while(getTile(x, leftY).id == TILE_NOTHING)
			{
				leftY++;
				if(getTile(x, leftY + 6).id == TILE_NOTHING) leftY += 6;
			}
			rightY = game.WORLD_HEIGHT / 5;
			while(getTile(x + width, rightY).id == TILE_NOTHING)
			{
				rightY++;
				if(getTile(x + width, rightY + 6).id == TILE_NOTHING) rightY += 6;
			}
			y = max(leftY, rightY);
			parabola(x, y, width, depth, TILE_WATER);
		}
	}

//	Gravitating Sand
	middleText("Gravitating Sand", updateProgress());
	if(surfaceChunk)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++)
		{
			for(int y = game.WORLD_HEIGHT - 1; y > 0; y--)
			{
				if(getTile(x, y).id == TILE_SAND && getTile(x, y + 1).id == TILE_NOTHING)
				{
					tempY = y;
					while(tempY < game.WORLD_HEIGHT - 1 && getTile(x, tempY + 1).id == TILE_NOTHING)
					{
						tempY++;
					}
					setTile(x, tempY, TILE_SAND);
					setTile(x, y, TILE_NOTHING);
				}
			}
		}
	}

//	Wetting mud
	middleText("Wet Jungle", updateProgress());
	if(surfaceChunk)
	{
		for(int x = 1; x < game.WORLD_WIDTH - 1; x++)
		{
			for(int y = game.WORLD_HEIGHT / 6; y < game.WORLD_HEIGHT / 2; y++)
			{
				if (getTile(x, y).id == TILE_MUD)
				{
					if(getTile(x + 1, y).id == TILE_NOTHING && randRange(0, 4) == 0)
					{
						setTile(x + 1, y, TILE_WATER);
					}
					if(getTile(x - 1, y).id == TILE_NOTHING && randRange(0, 4) == 0)
					{
						setTile(x - 1, y, TILE_WATER);
					}
					if(getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 4) == 0)
					{
						setTile(x, y - 1, TILE_WATER);
					}
				}
			}
		}
	}

//  Cobwebs
	middleText("Cobwebs", updateProgress());
	if(undergroundChunk)
	for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
	{
		for(int tries = 0; tries < 1000; tries++)
		{
			x = rand() % game.WORLD_WIDTH;
			y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);

			if(getTile(x, y).id != TILE_NOTHING && getTile(x, y).id != TILE_VINE && getTile(x, y).id != TILE_COBWEB) continue;

			int canPlace = 0, newY = 0;

			for(int j = y - 1; j >= 0; j--)
			{
				if(tiles[getTile(x, j).id].physics != PHYS_NON_SOLID)
				{
					canPlace = 1;
					break;
				}
				else
				{
					newY = j;
				}
			}

			if(canPlace)
			{
				clump(x, newY, poisson(9), TILE_COBWEB, false, 0, 0);
				break;
			}
		}
	}

//	Gems(had to do it in a separate step for... ummmm... reasons?)
	middleText("Gems", updateProgress());
	if(undergroundChunk)
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = game.WORLD_HEIGHT / 3.2; y < game.WORLD_HEIGHT; y++)
		{
			if((rand() % 10) == 0)
			{
				// Warning: This relies on gem IDs being next to each other, bad idea
				int gemID = TILE_AMETHYST + (rand() % 6);

				if(getTile(x, y).id == TILE_NOTHING)
				{
					if(getTile(x, y + 1).id == TILE_STONE || getTile(x - 1, y).id == TILE_STONE || getTile(x + 1, y).id == TILE_STONE)
					{
						setTile(x, y, gemID);
					}
				}
			}
		}
	}

//	Life Crystals
	middleText("Life Crystals", updateProgress());
	if(surfaceChunk)
	for(int i = 0; i < 40 * game.WORLDGEN_MULTIPLIER; i++)
	{
		check = (Item){ITEM_CRYST, PREFIX_NONE, 1};
		for(int try = 0; try < 50; try++)
		{
			x = randRange(25, game.WORLD_WIDTH - 25);
			y = randRange(game.WORLD_HEIGHT / 2.8, game.WORLD_HEIGHT - 10);
			if(getTile(x, y).id == TILE_NOTHING)
			{
				setTile(x + 1, y, TILE_NOTHING);
				setTile(x, y + 1, TILE_NOTHING);
				setTile(x + 1, y + 1, TILE_NOTHING);
				if(!checkArea(x, y, 2, 2, true))
				{
					setTile(x, y + 2, TILE_STONE);
					setTile(x + 1, y + 2, TILE_STONE);
				}
				placeTile(x, y, &check);
				break;
			}
		}
	}

//	Buried Chests
	middleText("Buried Chests", updateProgress());
	if(surfaceChunk)
	for(int i = 0; i < 30 * game.WORLDGEN_MULTIPLIER; i++)
	{
		placedChest = false;
		num = min(max(1, round(poisson(1.25))), 3);
		for(tries = 0; tries < 100; tries++)
		{
			x = randRange(25, game.WORLD_WIDTH - 25);
			y = randRange(game.WORLD_HEIGHT / 2.8, game.WORLD_HEIGHT);
			if(getTile(x, y).id == TILE_NOTHING) break;
		}
		if(tries == 100) continue;
		for(int room = 0; room < num; room++)
		{
			width = randRange(10, 20);
			box(x, y, width, 6, TILE_WOOD, 0.9, TILE_NOTHING);
			tempX = randRange(x + 1, x + width - 5);
			for(int dX = 0; dX < randRange(2, 5); dX++)
			{
				setTile(tempX + dX, y, TILE_PLATFORM);
			}
			if(!placedChest)
			{
				if(randRange(0, num) == 0 || room == num - 1)
				{
					tries = 0;
					check = (Item){ITEM_CHEST, PREFIX_NONE, 1};
					do
					{
						tempX = randRange(x, x + width - 2);
						placeTile(tempX, y + 3, &check);
						tries++;
					}
					while(check.amount == 1 && tries < 50);
					if(check.amount == 0)
					{
						placedChest = true;
						addLoot(world.chests.findChest(tempX, y + 3), TABLE_UNDERGROUND);
					}
				}
			}
			tempX = randFloat() < 0.5 ? x : x + width - 1;
			for(int dY = 2; dY < 5; dY++) setTile(tempX, y + dY, TILE_NOTHING);
			check = (Item){ITEM_DOOR, PREFIX_NONE, 1};
			placeTile(tempX, y + 2, &check);
			y += 7;
			x += randRange(-(width - 3), width - 3);
		}
	}

//	Surface Chests
	middleText("Surface Chests", updateProgress());
	if(surfaceChunk)
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		stage = 0;
		for(int y = 0; y < game.WORLD_HEIGHT / 2.5; y++)
		{
			if(getTile(x, y).id != TILE_NOTHING) stage = 1;
			else if(stage == 1)
			{
				if(rand() % 35 == 0)
				{
					check = (Item){ITEM_CHEST, PREFIX_NONE, 1};
					while(getTile(x, y + 2).id == TILE_NOTHING) y++;
					placeTile(x, y, &check);
					if(check.amount == 0)
					{
						addLoot(world.chests.findChest(x, y), TABLE_SURFACE);
					}
				}
				break;
			}
			
		}
	}

//	Pots
	middleText("Pots", updateProgress());
	if(surfaceChunk)
	{
		for(int x = 0; x < game.WORLD_WIDTH - 1; x++)
		{
			for(int y = game.WORLD_HEIGHT / 4; y < game.WORLD_HEIGHT; y++)
			{
				if (checkArea(x, y, 2, 2, true) && rand() % 15 == 0)
				{
					int canPlace = 0;

					for(int i = 0; i < y; i++)
					{
						if(tiles[getTile(x, i).id].physics != PHYS_NON_SOLID)
						{
							canPlace = 1;
							break;
						}
					}

					if(canPlace) placeTile(x, y, &(Item){ITEM_POT, PREFIX_NONE, 1});
					y += 3;
				}
			}
		}
	}

//	Spreading grass
	middleText("Spreading Grass", updateProgress());
	if(surfaceChunk)
	{
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT / 2.8; y++)
		{
			tile = getTile(x, y);
			if(tile.id == TILE_DIRT)
			{
				setTile(x, y, TILE_GRASS);
				if(x == 0 || x == game.WORLD_WIDTH - 1 || y == game.WORLD_HEIGHT) break;
				if(getTile(x - 1, y).id == TILE_NOTHING || getTile(x + 1, y).id == TILE_NOTHING)
				{
					setTile(x, y + 1, TILE_GRASS);
				}
				break;
			}
			else if(tile.id == TILE_MUD)
			{
				setTile(x, y, TILE_GRASS_JUNGLE);
				if(x == 0 || x == game.WORLD_WIDTH - 1 || y == game.WORLD_HEIGHT) break;
				if(getTile(x - 1, y).id == TILE_NOTHING || getTile(x + 1, y).id == TILE_NOTHING)
				{
					setTile(x, y + 1, TILE_GRASS_JUNGLE);
				}
				break;
			} else if(tile.id != TILE_NOTHING) break;
		}
	}
	}

//	Trees
	middleText("Planting Trees", updateProgress());
	if(surfaceChunk)
	{
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		if(rand() % 40 == 0)
		{
			copseHeight = rand() % 7 + 1;
			for(; rand() % 10 != 0 && x < game.WORLD_WIDTH; x += 4)
			{
				for(int y = 1; y < game.WORLD_HEIGHT; y++)
				{
					tile = getTile(x, y);
					if(tile.id == TILE_GRASS)
					{
						generateTree(x, y - 1, copseHeight);
						break;
					}
					else if(tile.id == TILE_GRASS_JUNGLE)
					{
						generateTree(x, y - 1, copseHeight + 4);
						break;
					}
					else if(tile.id != TILE_NOTHING) break;
				}
			}
		}
	}
	}

//	Weeds and cacti
	middleText("Weeds", updateProgress());
	if(surfaceChunk)
	{
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 1; y < game.WORLD_HEIGHT; y++)
		{
			if((getTile(x, y).id == TILE_GRASS || getTile(x, y).id == TILE_MUD) && getTile(x, y - 1).id == TILE_NOTHING && rand() % 4 > 0)
			{
				if(rand() % 8)
				{
					setTile(x, y - 1, TILE_PLANT);
				}
				else
				{
					setTile(x, y - 1, TILE_MUSHROOM);
				}
			}
		}
	}
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT; y++)
		{
			if(getTile(x, y).id == TILE_NOTHING) continue;
			else if(getTile(x, y).id == TILE_SAND)
			{
				y--;
				height = randRange(3, 7);
				if(checkArea(x - 2, y - height, 5, height + 1, false)) generateCactus(x, y, height);
				break;
			}
			else break;
		}
	}
	}

//	Vines
	middleText("Vines", updateProgress());
	if(surfaceChunk)
	{
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT - 1; y++)
		{
			tile = getTile(x, y);
			if(y < game.WORLD_HEIGHT / 3.2 && tile.id == TILE_GRASS && getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 3) > 0)
			{
				for(int dY = 1; dY < randRange(3, 11) && getTile(x, y + dY).id == TILE_NOTHING; dY++) setTile(x, y + dY, TILE_VINE);
			}
			else if(tile.id == TILE_GRASS_JUNGLE && getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 3) > 0)
			{
				for(int dY = 1; dY < randRange(3, 11) && getTile(x, y + dY).id == TILE_NOTHING; dY++) setTile(x, y + dY, TILE_VINE);
			}
		}
	}
	}

//	Sealing Water
//	Caves are carved before lakes are placed, so a body of water can end up sitting
//	directly above a cave void and drain away for good. Water can only move down,
//	down-diagonally or sideways, so plugging those neighbours with a non-floodable
//	tile keeps the body intact until the player digs it open. Only the surface half
//	of the world is handled, which is where lakes, oceans and the shallow caves are.
	middleText("Sealing Water", updateProgress());
	if(surfaceChunk)
	for(int y = 0; y < game.WORLD_HEIGHT / 2; y++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++)
		{
			if(getTile(x, y).id != TILE_WATER) continue;
//			Down, down-left and down-right. Sand still settles later, so look past it
			for(int dX = -1; dX <= 1; dX++)
			{
				int checkX = x + dX;
				if((unsigned)checkX >= (unsigned)game.WORLD_WIDTH) continue;
				for(int dY = y + 1; dY < min(y + 5, game.WORLD_HEIGHT); dY++)
				{
					Tile check = getTile(checkX, dY);
//					Still inside the body, its own water tiles handle this column
					if(check.id == TILE_WATER) break;
					if(tiles[check.id].physics == PHYS_SAND) continue;
					if(tiles[check.id].canFlood) setTile(checkX, dY, sealMaterial(checkX, dY - 1));
					break;
				}
			}
//			Same-row leaks, i.e. a cave mouth flush with the water surface
			if(x > 0 && tiles[getTile(x - 1, y).id].canFlood) setTile(x - 1, y, sealMaterial(x - 1, y - 1));
			if(x + 1 < game.WORLD_WIDTH && tiles[getTile(x + 1, y).id].canFlood) setTile(x + 1, y, sealMaterial(x + 1, y - 1));
		}
	}
}