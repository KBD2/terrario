#include <stdlib.h>
#include <math.h>

#include <gint/defs/util.h>

#include "generate.h"
#include "world.h"
#include "chest.h"

#define NUM_WORLD_GEN_PARTS 27
#define WORLD_SMOOTH_PASSES 5

GYRAM unsigned char yPositions[1000];

GYRAM Coords checkCoords[CHECK_BUFFER_SIZE];

struct ChestLootTable undergroundLoot = {
	.num = 4,
	.loot = (const ChestLoot[]){
//		 Num			   		 Items											Min	Max	Low	High
		{2,	(const enum Items[]){ITEM_CLOUD_BOTTLE, ITEM_MAGIC_MIRROR},			1,	1,	1,	5},
		{3,	(const enum Items[]){ITEM_IRON_BAR, ITEM_COPPER_BAR, ITEM_TIN_BAR},	5,	14,	1,	2},
		{1, (const enum Items[]){ITEM_COIN_SILVER},								0,  20,	1,	1},
		{1, (const enum Items[]){ITEM_LESSER_HEALING_POTION},					3,	5,	1,	2},
	}
};

struct ChestLootTable surfaceLoot = {
	.num = 4,
	.loot = (const ChestLoot[]){
//		 Num					 Items											Min	Max	Low	High
		{1,	(const enum Items[]){ITEM_AGLET},									1,	1,	1,	2},
		{3,	(const enum Items[]){ITEM_IRON_BAR, ITEM_COPPER_BAR, ITEM_TIN_BAR},	3,	10,	1,	2},
		{1, (const enum Items[]){ITEM_COIN_COPPER},								0,  30,	1,	1},
		{1, (const enum Items[]){ITEM_LESSER_HEALING_POTION},					3,	5,	1,	2},
	}
};

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
	int deltaY;
	int depth, leftY, rightY;

	middleText("Clearing", updateProgress());
	for(int y = 0; y < game.WORLD_HEIGHT; y++)
	{
		for(int x = 0; x < game.WORLD_WIDTH; x++) setTile(x, y, TILE_NOTHING);
	}

	middleText("Putting Dirt", updateProgress());
//	Dirt
	perlin(10, 30, game.WORLD_HEIGHT / 5, TILE_DIRT, 3);
//	Stone
	perlin(6, 20, game.WORLD_HEIGHT / 2.8, TILE_STONE, 1);

//	Tunnels
	middleText("Emptying Caves", updateProgress());
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

//	Mud
	middleText("Junglifying", updateProgress());
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
					setTile(x + dX, y, TILE_MUD);
					left--;
				}
				y++;
			}
		}
	}

//	Sand
	middleText("Desertifying", updateProgress());
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
	middleText("Rocking Dirt", updateProgress());
	for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % (int)(game.WORLD_HEIGHT / 2.8);
		if(getTile(x, y).id == TILE_DIRT || getTile(x, y).id == TILE_MUD)
		{
			clump(x, y, poisson(10), TILE_STONE, true, 0, 0);
		}
	}

//	Rocks in dirt
	middleText("Claying Dirt", updateProgress());
	for(int i = 0; i < 1000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % (int)(game.WORLD_HEIGHT / 4.2);
		if(getTile(x, y).id == TILE_DIRT)
		{
			clump(x, y, poisson(10), TILE_CLAY, true, 0, 0);
		}
	}

//	Dirt in rocks
	middleText("Dirting Rocks", updateProgress());
	for(int i = 0; i < 3000 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
		if(getTile(x, y).id == TILE_STONE)
		{
			clump(x, y, poisson(10), TILE_DIRT, true, 0, 0);
		}
	}

//	Small holes
	middleText("Cheeseholing", updateProgress());
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
	middleText("Caving", updateProgress());
	for(int i = 0; i < 150 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = randRange(game.WORLD_HEIGHT / 3.2, game.WORLD_HEIGHT);
		clump(x, y, poisson(200), TILE_NOTHING, true, 0, 0);
	}

//	Grass
	middleText("Growing Grass", updateProgress());
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

//	Iron
	middleText("Ironing", updateProgress());
	for(int i = 0; i < 500 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % game.WORLD_HEIGHT;
		if(getTile(x, y).id != TILE_SAND)
		{
			clump(x, y, poisson(6), TILE_IRON_ORE, true, 0, 0);
		}
	}

//	Copper
	middleText("Coppering", updateProgress());
	for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % game.WORLD_HEIGHT;
		if(getTile(x, y).id != TILE_SAND)
		{
			clump(x, y, poisson(8), TILE_COPPER_ORE, true, 0, 0);
		}
	}

//	Tin
	middleText("Tinning", updateProgress());
	for(int i = 0; i < 750 * game.WORLDGEN_MULTIPLIER; i++)
	{
		x = rand() % game.WORLD_WIDTH;
		y = rand() % game.WORLD_HEIGHT;
		if(getTile(x, y).id != TILE_SAND)
		{
			clump(x, y, poisson(10), TILE_TIN_ORE, true, 0, 0);
		}
	}

//	Lakes
	middleText("Lakifying", updateProgress());
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
	middleText("Beaching", updateProgress());
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

//	Smooth World
	middleText("Smoothing", updateProgress());
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
				if(tile.id == TILE_WATER && deltaY > 0)
				{
					for(int dY = 0; dY < deltaY; dY++) setTile(x, tempY + dY, TILE_NOTHING);
					ySave = tempY + deltaY - 1;
				}
				else if(deltaY > ((tile.id != TILE_SAND) ? 2 : 1) && getTile(x, tempY + 6).id != TILE_NOTHING)
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

//	Life Crystals
	middleText("Crystalyzing", updateProgress());
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
			}
			if(check.amount == 0) break;
		}
	}

//	Buried Chests
	middleText("Burying Chests", updateProgress());
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
	middleText("Chestifying", updateProgress());
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
			else if(tile.id != TILE_NOTHING) break;
		}
	}

//	Cacti
	middleText("Cactifying", updateProgress());
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
					else if(tile.id == TILE_MUD)
					{
						generateTree(x, y - 1, copseHeight + 4);
						break;
					}
					else if(tile.id != TILE_NOTHING) break;
				}
			}
		}
	}

//	Weeds
	middleText("Weedifying", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 1; y < game.WORLD_HEIGHT; y++)
		{
			if((getTile(x, y).id == TILE_GRASS || getTile(x, y).id == TILE_MUD) && getTile(x, y - 1).id == TILE_NOTHING && rand() % 4 > 0)
			{
				if(rand() % 6)
				{
					setTile(x, y - 1, TILE_MUSHROOM);
				}
				else
				{
					setTile(x, y - 1, TILE_PLANT);
				}
			}
		}
	}

//	Vines
	middleText("Vining", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT; y++)
		{
			if((getTile(x, y).id == TILE_GRASS || getTile(x, y).id == TILE_MUD) && getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 3) > 0)
			{
				for(int dY = 1; dY < randRange(3, 11) && getTile(x, y + dY).id == TILE_NOTHING; dY++) setTile(x, y + dY, TILE_VINE);
			}
		}
	}

//	Water on Mud
	middleText("Wetting Mud", updateProgress());
	for(int x = 0; x < game.WORLD_WIDTH; x++)
	{
		for(int y = 0; y < game.WORLD_HEIGHT / 2; y++)
		{
			if (getTile(x, y).id == TILE_MUD)
			{
				if(getTile(x + 1, y).id == TILE_NOTHING && randRange(0, 3) > 0)
				{
					setTile(x + 1, y, TILE_WATER);
				}
				if(getTile(x - 1, y).id == TILE_NOTHING && randRange(0, 3) > 0)
				{
					setTile(x - 1, y, TILE_WATER);
				}
				if(getTile(x, y + 1).id == TILE_NOTHING && randRange(0, 3) > 0)
				{
					setTile(x, y + 1, TILE_WATER);
				}
			}
		}
	}
}

void addLoot(struct Chest *chest, enum LootTables table)
{
	struct ChestLootTable *lootTable;
	int amount;
	const ChestLoot *currLoot;
	int slot = 0;
	enum Items item;

	switch(table) {
		case TABLE_UNDERGROUND:
			lootTable = &undergroundLoot;
			break;
		
		case TABLE_SURFACE:
			lootTable = &surfaceLoot;
			break;
		
		default:
			return;
	}

	for(int idx = 0; idx < lootTable->num; idx++)
	{
		currLoot = &lootTable->loot[idx];
		if(rand() % currLoot->ratioHigh >= currLoot->ratioLow - 1)
		{
			amount = (rand() % (currLoot->amountMax - currLoot->amountMin + 1)) + currLoot->amountMin;
			item = currLoot->items[rand() % currLoot->num];
			chest->items[slot] = (Item){item, rand() % PREFIX_COUNT, amount};
			slot++;
		}
	}
}
