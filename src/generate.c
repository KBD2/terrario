#include <gint/std/stdlib.h>
#include <gint/defs/util.h>
#include "math.h"

#include "generate.h"
#include "world.h"
#include "chest.h"

#define NUM_WORLD_GEN_PARTS 13

GXRAM struct Coords clumpCoords[WORLD_CLUMP_BUFFER_SIZE];
unsigned char *yPositions;

int updateProgress()
{
	static int progress = 0;

	progress += 100 / NUM_WORLD_GEN_PARTS;

	return progress;
}

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

void perlin(int amplitude, int wavelength, int baseY, enum Tiles tile, int iterations)
{
	int perlinY;
	float a;
	float b;

	for(int x = 0; x < game.WORLD_WIDTH; x++) yPositions[x] = baseY;

	for(int i = 0; i < iterations; i++)
	{
		a = randFloat();
		b = randFloat();
		for(int x = 0; x < game.WORLD_WIDTH; x++)
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
		amplitude >>= 1;
	}

	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = yPositions[x]; y < game.WORLD_HEIGHT; y++)
		{
			setTile(x, y, tile, makeVar());
		}
	}
}

void clump(int x, int y, int num, enum Tiles tile, bool maskEmpty, float skewHorizontal, float skewVertical)
{
	int end = 1;
	int selected;
	struct Coords selectedTile;
	int deltas[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	Tile tileCheck;
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
		setTile(selectedTile.x, selectedTile.y, tile, makeVar());
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
			clumpCoords[end] = (struct Coords){checkX, checkY};
			end++;
			if(end >= WORLD_CLUMP_BUFFER_SIZE) end = 0;
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
					setTile(x + dX, y + dY, tile, makeVar());
				}
				else{
					setTile(x + dX, y + dY, swap, makeVar());
				}
			}
			else
			{
				setTile(x + dX, y + dY, TILE_NOTHING, 0);
			}
		}
	}
}

void generateWorld()
{
	int x, y;
	Tile tile;
	int copseHeight;
	int tunnelYPositions[10];
	int num, width, tempX, tries;
	bool placedChest;
	Item check;

	yPositions = malloc(game.WORLD_WIDTH * sizeof(unsigned char));

	middleText("Reset", updateProgress());
	for(int y = 0; y < game.WORLD_HEIGHT; y++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++) setTile(x, y, TILE_NOTHING, 0);
	}

	middleText("Terrain", updateProgress());

//	Dirt
	perlin(10, 30, game.WORLD_HEIGHT / 5, TILE_DIRT, 3);

//	Stone
	perlin(6, 20, game.WORLD_HEIGHT / 2.8, TILE_STONE, 1);

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

//	Rocks in dirt
	middleText("Rocks In Dirt", updateProgress());
	for(int i = 0; i < 1000; i++)
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
	for(int i = 0; i < 3000; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = min((rand() % (int)(game.WORLD_HEIGHT - game.WORLD_HEIGHT / 2.8)) + game.WORLD_HEIGHT / 2.8, game.WORLD_HEIGHT - 1);
		if(getTile(x, y).id == TILE_STONE)
		{
			clump(x, y, poisson(10), TILE_DIRT, true, 0, 0);
		}
	}

//	Small holes
	middleText("Small Holes", updateProgress());
	for(int i = 0; i < 750; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = min((rand() % (int)(game.WORLD_HEIGHT - game.WORLD_HEIGHT / 4)) + game.WORLD_HEIGHT / 4, game.WORLD_HEIGHT - 1);
		clump(x, y, poisson(25), TILE_NOTHING, true, 0, 0);
	}

//	Caves
	middleText("Caves", updateProgress());
	for(int i = 0; i < 150; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = min((rand() % (int)(game.WORLD_HEIGHT - game.WORLD_HEIGHT / 3.5)) + game.WORLD_HEIGHT / 3.5, game.WORLD_HEIGHT - 1);
		clump(x, y, poisson(200), TILE_NOTHING, true, 0, 0);
	}

//	Grass
	middleText("Grass", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT; y++)
		{
			tile = getTile(x, y);
			if(tile.id == TILE_DIRT)
			{
				tile.id = TILE_GRASS;
				if(x == 0 || x == game.WORLD_WIDTH - 1 || y == game.WORLD_HEIGHT) break;
				if(getTile(x - 1, y).id == TILE_NOTHING || getTile(x + 1, y).id == TILE_NOTHING)
				{
					setTile(x, y + 1, TILE_GRASS, makeVar());
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
			if(getTile(x, y).id == TILE_DIRT && findState(x, y) != 15)
			{
				setTile(x, y, TILE_GRASS, makeVar());
			}
		}
	}

//	Shinies
	middleText("Shinies", updateProgress());
	for(int i = 0; i < 750; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % game.WORLD_HEIGHT;
		clump(x, y, poisson(10), TILE_IRON_ORE, true, 0, 0);
	}

//	Buried Chests
	middleText("Buried Chests", updateProgress());
	for(int i = 0; i < 30; i++)
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
				setTile(tempX + dX, y, TILE_PLATFORM, 0);
			}
			if(!placedChest)
			{
				if(randRange(0, num) == 0 || room == num - 1)
				{
					tries = 0;
					check = (Item){ITEM_CHEST, 1};
					do
					{
						tempX = randRange(x, x + width - 2);
						placeTile(tempX, y + 3, &check);
						tries++;
					}
					while(check.amount == 1 && tries < 50);
					if(check.amount == 0) placedChest = true;
				}
			}
			tempX = randFloat() < 0.5 ? x : x + width - 1;
			for(int dY = 2; dY < 5; dY++) setTile(tempX, y + dY, TILE_NOTHING, 0);
			check = (Item){ITEM_DOOR, 1};
			placeTile(tempX, y + 2, &check);
			y += 7;
			x += randRange(-(width - 3), width - 3);
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
					else if(tile.id != TILE_NOTHING) break;
				}
			}
		}
	}

//	Weeds
	middleText("Weeds", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 1; y < game.WORLD_HEIGHT; y++)
		{
			if(getTile(x, y).id == TILE_GRASS && getTile(x, y - 1).id == TILE_NOTHING && rand() % 4 > 0) setTile(x, y - 1, TILE_PLANT, makeVar());;
		}
	}

//	Vines
	middleText("Vines", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_WIDTH / 4.5; y++)
		{
			if(getTile(x, y).id == TILE_GRASS && getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 3) > 0)
			{
				for(int dY = 1; dY < randRange(3, 11) && getTile(x, y + dY).id == TILE_NOTHING; dY++) setTile(x, y + dY, TILE_VINE, rand() % 4);
			}
		}
	}

	free(yPositions);
}