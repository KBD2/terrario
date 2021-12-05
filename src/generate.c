#include <stdlib.h>
#include <math.h>

#include <gint/defs/util.h>

#include "generate.h"
#include "world.h"
#include "chest.h"

#define NUM_WORLD_GEN_PARTS 28
#define WORLD_SMOOTH_PASSES 5

GYRAM unsigned char yPositions[1000];

GYRAM Coords checkCoords[CHECK_BUFFER_SIZE];

const float rand_dir[] = {
	1, 0,
	0.9807852804032304, 0.19509032201612825,
	0.9238795325112867, 0.3826834323650898,
	0.8314696123025452, 0.5555702330196022,
	0.7071067811865476, 0.7071067811865475,
	0.5555702330196023, 0.8314696123025452,
	0.38268343236508984, 0.9238795325112867,
	0.19509032201612833, 0.9807852804032304,
	0, 1,
	-0.1950903220161282, 0.9807852804032304,
	-0.3826834323650897, 0.9238795325112867,
	-0.555570233019602, 0.8314696123025455,
	-0.7071067811865475, 0.7071067811865476,
	-0.8314696123025453, 0.5555702330196022,
	-0.9238795325112867, 0.3826834323650899,
	-0.9807852804032304, 0.1950903220161286,
	-1, 0,
	-0.9807852804032304, -0.19509032201612836,
	-0.9238795325112868, -0.38268343236508967,
	-0.8314696123025455, -0.555570233019602,
	-0.7071067811865477, -0.7071067811865475,
	-0.5555702330196022, -0.8314696123025452,
	-0.38268343236509034, -0.9238795325112865,
	-0.19509032201612866, -0.9807852804032303,
	0, -1,
	0.1950903220161283, -0.9807852804032304,
	0.38268343236509, -0.9238795325112866,
	0.5555702330196018, -0.8314696123025455,
	0.7071067811865474, -0.7071067811865477,
	0.8314696123025452, -0.5555702330196022,
	0.9238795325112865, -0.3826834323650904,
	0.9807852804032303, -0.19509032201612872
};

static float progress = 0;

static int seed_3a = 1;
static int seed_3b = 1;
static int seed_3c = 1;

#define fast_sin(num) (fast_cos((num) + 1.5f * PI))
#define fast_abs(num) ((num) < 0 ? -(num) : (num))

float fast_cos(float x) {
  float tp = 1 / (2 * PI);

  x = fast_abs(x * tp);
  x -= 0.25f + (int)(x + 0.25f);
  x *= 16.0f * (fast_abs(x) - 0.5f);

  // optional
  // x += 0.225f * x * (fast_abs(x) - 1.0f);

  return x;
}

int updateProgress()
{
	progress += 100.0 / NUM_WORLD_GEN_PARTS;

	return (int)(progress);
}

int rand_3() {
  seed_3a = (seed_3a * 1664525 + 1013904223) % 1431655765;
  seed_3b = (seed_3b * 16843019 + 826366249) % 1431655765;
  seed_3c = (seed_3c * 16843031 + 826366237) % 1431655765;

  return seed_3a + seed_3b + seed_3c;
}

void srand_3(int x, int y) {
	seed_3a = (      x * 1664525 + 1013904223) % 1431655765;
	seed_3b = (seed_3a * 16843019 + 826366249) % 1431655765;
	
	seed_3b = (      y * 16843019 + 826366249) % 1431655765;
	seed_3a = (seed_3b * 1664525 + 1013904223) % 1431655765;
}

float randFloat()
{
	return (float)rand_3() / 0x7fffffff;
}

int randRange(int low, int high)
{
	return (rand_3() % (high - low)) + low;
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

float lerp(float x, float y, float w)
{
	if (w < 0) return x;
	if (w > 1) return y;

	return (y - x) * w + x;
	// return (y - x) * ((w * (w * 6.0f - 15.0f) + 10.0f) * w * w * w) + x;
}

float dot_grad(int tile_x, int tile_y, float x, float y)
{
	srand_3(tile_x, tile_y);
	int angle = rand_3() & 31;
	
	return (x - tile_x) * rand_dir[angle * 2 + 0] + (y - tile_y) * rand_dir[angle * 2 + 1];
}

float perlin(float x, float y)
{
	float a = lerp(dot_grad((int)(x + 0), (int)(y + 0), x, y), dot_grad((int)(x + 1), (int)(y + 0), x, y), x - (int)(x));
	float b = lerp(dot_grad((int)(x + 0), (int)(y + 1), x, y), dot_grad((int)(x + 1), (int)(y + 1), x, y), x - (int)(x));
	
	return lerp(a, b, y - (int)(y));
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
			checkCoords[end] = (Coords){checkX, checkY};
			end++;
			if(end >= CHECK_BUFFER_SIZE) end = 0;
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
	float mul;
	int ySave = 0;
	int deltaX, deltaY;
	int depth, leftY, rightY;

	middleText("Reset", updateProgress());
	for(int y = 0; y < game.WORLD_HEIGHT; y++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++) setTile(x, y, TILE_NOTHING);
	}

	middleText("Terrain", updateProgress());
	for (int j = 0; j < game.WORLD_WIDTH; j++)
	{
		char buffer[30];
		sprintf(buffer, "Terrain(%d)", j);
		middleText(buffer, (int)(progress));
		
	    float sh = (perlin(j / 60.0f, 3.1f) + 1) * 30.0f + (perlin(j / 30.0f, 4.1) + 1) * 15.0f + (perlin(j / 6.0f, 5.1f) + 1) * 2.5f;
	    float dh = sh + ((perlin(j / 60.0f, 6.1f) + 1) * 20.0f + (perlin(j / 30.0f, 7.1f) + 1) * 10.0f + (perlin(j / 6.0f, 8.1f) + 1) * 2.0f);
	    
	    for (int i = 0; i < game.WORLD_HEIGHT; i++)
	    {
	        enum Tiles t = TILE_DIRT;
	        
	        if (sh <= i) {
	            if (dh <= i) t = TILE_STONE;

				setTile(j, i, t);
				continue;

	            if (perlin(j / 32.0f, i / 32.0f) < lerp(0.3f, 0.0f, i / 100.0f))
	            {
	                setTile(j, i, t);
	                continue;
	            }

	            // if (perlin(j / 24.0f, i / 8.0f) >= 0.1f + lerp(0.1f, 0.0f, i / 175.0f)) continue;
	            
	            float noise = 0.0f;
	            float detail = 30.0f;
	            
	            while (detail >= 15.0f)
	            {
	                noise += perlin(j / detail, i / detail) * detail;
	                detail /= 2.0f;
	            }
	            
	            noise /= 30.0f;

	            float warp = ((fast_sin((j + 90.0f * noise) / 12.0f) + 1) * 0.5f);
	            
	            float value = (int)(warp * warp * 256.0f);
	            value = (value >= 192 + lerp(32, 0, i / 175.0f)) ? 255 : 0;
	            
	            if (value == 0) setTile(j, i, t);
	    	}
		}
	}

	return;

//	Tunnels
	middleText("Tunnels", updateProgress());
	x = 100;
	while(x < game.WORLD_WIDTH - 100)
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
			x += 100;
		}
		else x++;
	}

//	Sand
	middleText("Sand", updateProgress());
	for(int i = 0; i < 40 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 3.5, game.WORLD_HEIGHT / 2);
		clump(x, y, poisson(40), TILE_SAND, true, 0, 0);
	}
	for(int i = 0; i < max(2, poisson(3)); i++)
	{
		width = poisson(60) * game.WORLDGEN_MULTIPLIER;
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

//	Rocks in dirt
	middleText("Rocks In Dirt", updateProgress());
	for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % (int)(game.WORLD_HEIGHT / 2.8);
		if(getTile(x, y).id == TILE_DIRT)
		{
			clump(x, y, poisson(10), TILE_STONE, true, 0, 0);
		}
	}

//	Dirt in rocks
	middleText("Dirt In Rocks", updateProgress());
	for(int i = 0; i < 3000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
		if(getTile(x, y).id == TILE_STONE)
		{
			clump(x, y, poisson(10), TILE_DIRT, true, 0, 0);
		}
	}

//	Clay
	middleText("Clay", updateProgress());
	for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % (int)(game.WORLD_HEIGHT / 4.2);
		if(getTile(x, y).id == TILE_DIRT)
		{
			clump(x, y, poisson(10), TILE_CLAY, true, 0, 0);
		}
	}

//	Small holes
	middleText("Small Holes", updateProgress());
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

//	Caves
	middleText("Caves", updateProgress());
	for(int i = 0; i < 150 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
		clump(x, y, poisson(200), TILE_NOTHING, true, 0, 0);
	}

//	Surface Caves
	middleText("Surface Caves", updateProgress());
	for(int i = 0; i < 150 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 4, game.WORLD_HEIGHT / 2.8);
		clump(x, y, poisson(125), TILE_NOTHING, true, 0, 0);
	}

//	Grass
	middleText("Grass", updateProgress());
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

//	Jungle
	middleText("Jungle", updateProgress());
	width = poisson(150) * game.WORLDGEN_MULTIPLIER;
	x = (rand() % 2) ? game.WORLD_WIDTH / 5 : game.WORLD_WIDTH - game.WORLD_WIDTH / 5 - width;
	for(int y = 0; y < game.WORLD_HEIGHT; y++)
	{
		deltaX = 0;
		while(getTile(x + deltaX, y).id == TILE_NOTHING) deltaX++;
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
			if(tile.id == TILE_MUD && findState(x, y != 0xf)) setTile(x, y, TILE_GRASS_JUNGLE);
		}
	}

//	Shinies
	middleText("Shinies", updateProgress());
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

//	Lakes
	middleText("Lakes", updateProgress());
	for(int i = 0; i < max(2, poisson(3)); i++)
	{
		x = randRange(75, game.WORLD_WIDTH - 75);
		width = max(15, poisson(15));
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

//	Beaches
	middleText("Beaches", updateProgress());
	leftY = 0;
	while(getTile(60 * game.WORLDGEN_MULTIPLIER, leftY).id == TILE_NOTHING) leftY++;
	rightY = 0;
	while(getTile(game.WORLD_WIDTH - 60 * game.WORLDGEN_MULTIPLIER, rightY).id == TILE_NOTHING) rightY++;
//	Left beach
	parabola(-62 * game.WORLDGEN_MULTIPLIER, leftY, 124 * game.WORLDGEN_MULTIPLIER, 32, TILE_DIRT);
	parabola(-60 * game.WORLDGEN_MULTIPLIER, leftY, 120 * game.WORLDGEN_MULTIPLIER, 30, TILE_SAND);
    parabola(-50 * game.WORLDGEN_MULTIPLIER, leftY + 2, 100 * game.WORLDGEN_MULTIPLIER, 20, TILE_WATER);
//	Right beach
	parabola(game.WORLD_WIDTH - 62 * game.WORLDGEN_MULTIPLIER, leftY, 124 * game.WORLDGEN_MULTIPLIER, 32, TILE_DIRT);
	parabola(game.WORLD_WIDTH - 60 * game.WORLDGEN_MULTIPLIER, leftY, 120 * game.WORLDGEN_MULTIPLIER, 30, TILE_SAND);
    parabola(game.WORLD_WIDTH - 50 * game.WORLDGEN_MULTIPLIER, leftY + 2, 100 * game.WORLDGEN_MULTIPLIER, 20, TILE_WATER);

//	Gravitating Sand
	middleText("Gravitating Sand", updateProgress());
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

//	Wetting mud
	middleText("Wet Jungle", updateProgress());
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

//	Smooth World
	middleText("Smooth World", updateProgress());
	for(int i = 0; i < WORLD_SMOOTH_PASSES; i++)
	{
//		Forward and reverse pass
		for(int pass = 0; pass < 2; pass++)
		{
			for(int y = 0; y < game.WORLD_HEIGHT; y++)
			{
				if(getTile(pass ? game.WORLD_WIDTH - 1 : 0, y).id != TILE_NOTHING)
				{
					ySave = y;
					break;
				}
			}
			for(int x = pass ? game.WORLD_WIDTH - 1 : 0; pass ? (x > -1) : (x < game.WORLD_WIDTH); x += pass ? -1 : 1)
			{
				tempY = 0;
				while(getTile(x, tempY).id == TILE_NOTHING) tempY++;
				deltaY = ySave - tempY;
				tile = getTile(x, tempY);
				if(deltaY > ((tile.id != TILE_SAND) ? 2 : 1) && getTile(x, tempY + 6).id != TILE_NOTHING)
				{
					for(int dY = 0; dY < min(deltaY - 1, 2); dY++)
					{
						setTile(x, tempY + dY, TILE_NOTHING);
					}
					ySave = tempY + deltaY - 1;
				}
				else ySave = tempY;
			}
		}
	}

//  Cobwebs
	middleText("Cobwebs", updateProgress());
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
	for(int i = 0; i < 30 * game.WORLDGEN_MULTIPLIER; i++)
	{
		placedChest = false;
		num = min(max(1, round(poisson(1.25))), 3);
		do
		{
			x = randRange(25, game.WORLD_WIDTH - 25);
			y = randRange(game.WORLD_HEIGHT / 2.8, game.WORLD_HEIGHT);
		}
		while(getTile(x, y).id != TILE_NOTHING);
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
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		stage = 0;
		for(int y = 0; y < game.WORLD_HEIGHT / 4; y++)
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

//	Spreading grass
	middleText("Spreading Grass", updateProgress());
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

//	Trees
	middleText("Planting Trees", updateProgress());
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

//	Weeds and cacti
	middleText("Weeds", updateProgress());
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

//	Vines
	middleText("Vines", updateProgress());
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
