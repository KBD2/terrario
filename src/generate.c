#include <gint/std/stdlib.h>
#include <gint/defs/util.h>
#include "math.h"

#include "generate.h"
#include "world.h"

struct Coords *clumpCoords;
int *yPositions;

float interpolate(float a, float b, float x){
	float f = (1.0 - cosf(x * PI)) * 0.5;
    return a * (1.0 - f) + b * f;
}

float randFloat()
{
	return (float)rand() / 0x7fffffff;
}

int randRange(int low, int high)
{
	return (rand() % (high - low)) + low;
}

int poisson(int lambda)
{
	int k = 0;
	float p = 1;
	double L = pow(E, -(double)lambda);
	while(p > L)
	{
		k++;
		p *= randFloat();
	}
	return k - 1;
}

void perlin(int amplitude, int wavelength, int baseY, enum Tiles tile, int iterations)
{
	int perlinY;
	float a;
	float b;

	for(int x = 0; x < WORLD_WIDTH; x++) yPositions[x] = baseY;

	for(int i = 0; i < iterations; i++)
	{
		a = randFloat();
		b = randFloat();
		for(int x = 0; x < WORLD_WIDTH; x++)
		{
			if(x % wavelength == 0)
			{
				a = b;
				b = randFloat();
				perlinY = a * amplitude;
			}
			else
			{
				perlinY = interpolate(a, b, (float)(x % wavelength) / wavelength) * amplitude;
			}
			yPositions[x] += perlinY;
		}
		wavelength >>= 1;
	}

	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = yPositions[x]; y < WORLD_HEIGHT; y++)
		{
			getTile(x, y) = (Tile){tile, makeVar()};
		}
	}
}

void clump(int x, int y, int num, enum Tiles tile, bool maskEmpty, float skewHorizontal, float skewVertical)
{
	int end = 1;
	int selected;
	struct Coords selectedTile;
	int deltas[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	Tile* tileCheck;
	int checkX, checkY;

	if(maskEmpty && getTile(x, y).id == TILE_NOTHING) return;

	clumpCoords[0] = (struct Coords){x, y};

	while(num > 0)
	{
		if(end == 0) return;
		selected = (unsigned int)rand() % end;
		selectedTile = clumpCoords[selected];
		clumpCoords[selected] = clumpCoords[end - 1];
		end--;
		getTile(selectedTile.x, selectedTile.y) = (Tile){tile, makeVar()};
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
			if(checkX < 0 || checkX >= WORLD_WIDTH || checkY < 0 || checkY >= WORLD_HEIGHT) continue;
			tileCheck = &getTile(checkX, checkY);
			if(tileCheck->id == tile || (maskEmpty && tileCheck->id == TILE_NOTHING)) continue;
			clumpCoords[end] = (struct Coords){checkX, checkY};
			end++;
			if(end >= WORLD_CLUMP_BUFFER_SIZE) end = 0;
		}
	}
}

void generateWorld()
{
	int x, y;
	Tile* tile;
	int copseHeight;
	int tunnelYPositions[10];

	clumpCoords = malloc(WORLD_CLUMP_BUFFER_SIZE * sizeof(struct Coords));
	allocCheck(clumpCoords);

	yPositions = malloc(WORLD_WIDTH * sizeof(int));
	allocCheck(yPositions);

	middleText("Terrain");
	
//	Dirt
	perlin(10, 40, WORLD_HEIGHT / 5, TILE_DIRT, 3);

//	Stone
	perlin(6, 20, WORLD_HEIGHT / 2.8, TILE_STONE, 1);

//	Tunnels
	middleText("Tunnels");
	x = 0;
	while(x < WORLD_WIDTH - 50)
	{
		if(randFloat() < 0.005)
		{
			for(int dX = 0; dX < 50; dX += 5)
			{
				y = 0;
				while(getTile(x + dX, y + 8).id != TILE_DIRT) y++;
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

//	Rocks in dirt
	middleText("Rocks In Dirt");
	for(int i = 0; i < 1000; i++)
	{
		x = rand() % WORLD_WIDTH;
		y = rand() % (int)(WORLD_HEIGHT / 2.8);
		if(getTile(x, y).id == TILE_DIRT)
		{
			clump(x, y, poisson(10), TILE_STONE, true, 0, 0);
		}
	}

//	Dirt in rocks
	middleText("Dirt In Rocks");
	for(int i = 0; i < 3000; i++)
	{
		x = rand() % WORLD_WIDTH;
		y = min((rand() % (int)(WORLD_HEIGHT - WORLD_HEIGHT / 2.8)) + WORLD_HEIGHT / 2.8, WORLD_HEIGHT - 1);
		if(getTile(x, y).id == TILE_STONE)
		{
			clump(x, y, poisson(10), TILE_DIRT, true, 0, 0);
		}
	}

//	Small holes
	middleText("Small Holes");
	for(int i = 0; i < 750; i++)
	{
		x = rand() % WORLD_WIDTH;
		y = min((rand() % (int)(WORLD_HEIGHT - WORLD_HEIGHT / 4)) + WORLD_HEIGHT / 4, WORLD_HEIGHT - 1);
		clump(x, y, poisson(25), TILE_NOTHING, true, 0, 0);
	}

//	Caves
	middleText("Caves");
	for(int i = 0; i < 150; i++)
	{
		x = rand() % WORLD_WIDTH;
		y = min((rand() % (int)(WORLD_HEIGHT - WORLD_HEIGHT / 3.5)) + WORLD_HEIGHT / 3.5, WORLD_HEIGHT - 1);
		clump(x, y, poisson(200), TILE_NOTHING, true, 0, 0);
	}

//	Grass
	middleText("Grass");
	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 0; y < WORLD_HEIGHT; y++)
		{
			tile = &getTile(x, y);
			if(tile->id == TILE_DIRT)
			{
				tile->id = TILE_GRASS;
				if(x == 0 || x == WORLD_WIDTH - 1 || y == WORLD_HEIGHT) break;
				if(getTile(x - 1, y).id == TILE_NOTHING || getTile(x + 1, y).id == TILE_NOTHING)
				{
					getTile(x, y + 1).id = TILE_GRASS;
				}
				break;
			}
			else if(tile->id != TILE_NOTHING) break;
		}
	}

	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 0; y < WORLD_HEIGHT / 2.8; y++)
		{
			if(getTile(x, y).id == TILE_DIRT && findState(x, y) != 15)
			{
				getTile(x, y).id = TILE_GRASS;
			}
		}
	}

//	Shinies
	middleText("Shinies");
	for(int i = 0; i < 750; i++)
	{
		x = rand() % WORLD_WIDTH;
		y = rand() % WORLD_HEIGHT;
		clump(x, y, poisson(10), TILE_IRON_ORE, true, 0, 0);
	}

//	Trees
	middleText("Planting Trees");
	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		if(rand() % 40 == 0)
		{
			copseHeight = rand() % 7 + 1;
			for(; rand() % 10 != 0 && x < WORLD_WIDTH; x += 4)
			{
				for(int y = 1; y < WORLD_WIDTH; y++)
				{
					tile = &getTile(x, y);
					if(tile->id == TILE_GRASS)
					{
						generateTree(x, y - 1, copseHeight);
						break;
					}
					else if(tile->id != TILE_NOTHING) break;
				}
			}
		}
	}

//	Weeds
	middleText("Weeds");
	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 1; y < WORLD_HEIGHT; y++)
		{
			if(getTile(x, y).id == TILE_GRASS && getTile(x, y - 1).id == TILE_NOTHING && rand() % 4 > 0) getTile(x, y - 1) = (Tile){TILE_PLANT, makeVar()};
		}
	}

//	Vines
	middleText("Vines");
	for(int x = 0; x < WORLD_WIDTH; x++)
	{
		for(int y = 0; y < WORLD_WIDTH / 4.5; y++)
		{
			if(getTile(x, y).id == TILE_GRASS && getTile(x, y + 1).id == TILE_NOTHING)
			{
				for(int dY = 1; dY < 11 && getTile(x, y + dY).id == TILE_NOTHING; dY++) getTile(x, y + dY) = (Tile){TILE_VINE, rand() % 4};
			}
		}
	}

	free(clumpCoords);
}