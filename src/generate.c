#include <stdlib.h>
#include <math.h>

#include <gint/defs/util.h>

#include "generate.h"
#include "world.h"
#include "chest.h"

#include <stdio.h>

#define WORLD_SEED 0xDEADBEEF

#define NUM_WORLD_GEN_PARTS (game.WORLD_WIDTH + 75 + 75 + 30 + game.WORLD_HEIGHT + 300 + game.WORLD_WIDTH + 150 + 1 + game.WORLD_WIDTH + 1)

GYRAM Coords checkCoords[CHECK_BUFFER_SIZE];

uint32_t rand_seed = 1;

int updateProgress()
{
	static float progress = 0;

	progress += 100.0 / NUM_WORLD_GEN_PARTS;

	return (int)(progress);
}

uint32_t fast_rand(void) {
  rand_seed ^= rand_seed >> 17;
  rand_seed *= 0xED5AD4BB;
  rand_seed ^= rand_seed >> 11;
  rand_seed *= 0xAC4C1B51;
  rand_seed ^= rand_seed >> 15;
  rand_seed *= 0x31848BAB;
  rand_seed ^= rand_seed >> 14;
  
  return rand_seed;
}

void fast_srand(uint32_t seed) {
  srand(seed);
  rand_seed = (seed + WORLD_SEED + 1);
}

float lerp(float x, float y, float w) {
  if (w < 0.0f) return x;
  if (w > 1.0f) return y;
  
  return (y - x) * w + x;
}

float lerp_smooth(float x, float y, float w) {
  if (w < 0.0f) return x;
  if (w > 1.0f) return y;
  
  return (y - x) * ((w * (w * 6.0f - 15.0f) + 10.0f) * w * w * w) + x;
}

float grad(int x, int y) {
  fast_srand(x + 1048577 * y);
  return (fast_rand() % 65537) / 65537.0f;
}

float noise_1(float x) {
  return lerp(grad((int)(x + 0), 1024), grad((int)(x + 1), 1024), x - (int)(x));
}

float noise_2(float x, float y) {
  x -= (y / 1.6f);
  x += 1024;
  
  float dx = x - (int)(x);
  float dy = y - (int)(y);
  
  if (1 - dx < dy) {
    float s = ((dx - dy) + 1) / 2;
    float h = 2 - dx - dy;
    
    float t = lerp(grad((int)(x + 0), (int)(y + 1)), grad((int)(x + 1), (int)(y + 0)), s);
    return lerp(grad((int)(x + 1), (int)(y + 1)), t, h);
  } else {
    float s = ((dx - dy) + 1) / 2;
    float h = dx + dy;
    
    float t = lerp(grad((int)(x + 0), (int)(y + 1)), grad((int)(x + 1), (int)(y + 0)), s);
    return lerp(grad((int)(x + 0), (int)(y + 0)), t, h);
  }
}

int clump(int x, int y, int radius, enum Tiles tile, int invert, int mask_empty, float off_x, float off_y) {
  if (mask_empty && (invert ? (getTile(x, y).id != TILE_NOTHING) : (getTile(x, y).id == TILE_NOTHING))) return 0;
  
  for (int dx = x - radius / 2; dx <= x + radius / 2; dx++) {
    if (dx >= game.WORLD_WIDTH) break;
    if (dx < 0) continue;
    
    for (int dy = y - radius / 2; dy <= y + radius / 2; dy++) {
      if (dy >= game.WORLD_WIDTH) break;
      if (dy < 0) continue;
      
      int dist = (dx - x) * (dx - x) + (dy - y) * (dy - y);
      
      if (dist > radius) {
        if (dy >= y) break;
        else continue;
      }
      
      if (mask_empty && (invert ? (getTile(dx, dy).id != TILE_NOTHING) : (getTile(dx, dy).id == TILE_NOTHING))) continue;
      if (noise_2(dx / 3.0f + off_x, dy / 3.0f + off_y) < 0.5f) continue;
      
      setTile(dx, dy, tile);
    }
  }
  
  return 1;
}

int unflood(int x, int y, enum Tiles tile) {
  if (x < 0 || y < 0 || x >= game.WORLD_WIDTH || y >= game.WORLD_HEIGHT) return 0;
  if (getTile(x, y).id != tile) return 0;
  
  setTile(x, y, TILE_NOTHING);
  
  return unflood(x - 1, y, tile) + unflood(x + 1, y, tile) + unflood(x, y + 1, tile) + 1;
}

int flood(int x, int y, enum Tiles tile) {
  if (x < 0 || y < 0 || x >= game.WORLD_WIDTH || y >= game.WORLD_HEIGHT) return 0;
  if (getTile(x, y).id != TILE_NOTHING) return 0;
  
  setTile(x, y, tile);
  
  return flood(x - 1, y, tile) + flood(x + 1, y, tile) + flood(x, y + 1, tile) + 1;
}

/*
Keeping generation step names the same as Terraria just to make it easier to follow its world gen process
*/
void generateWorld()
{
	fast_srand(0);
	
	int sand_x = fast_rand() % 4;
	  if (sand_x >= 2) sand_x++;
	  
	  int snow_x = 4 - sand_x;
	  int jungle_x = fast_rand() % 5;
	  
	  while (jungle_x == sand_x || jungle_x == snow_x) snow_x = fast_rand() % 5;
	  
	  sand_x = 90 + ((sand_x * 2 + 1) * (game.WORLD_WIDTH - 180)) / 10;
	  snow_x = 90 + ((snow_x * 2 + 1) * (game.WORLD_WIDTH - 180)) / 10;
	  jungle_x = 90 + ((jungle_x * 2 + 1) * (game.WORLD_WIDTH - 180)) / 10;
	  
	  int sand_width = ((game.WORLD_WIDTH - 180) / 5) - 25 - (fast_rand() % 10);
	  int snow_width = ((game.WORLD_WIDTH - 180) / 5) - 35 - (fast_rand() % 10);
	  int jungle_width = ((game.WORLD_WIDTH - 180) / 5) - (fast_rand() % 10);
	  
	  for (int i = 0; i < game.WORLD_WIDTH; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Terrain(%d/%d)", i, game.WORLD_WIDTH);
		middleText(buffer, updateProgress());
	  	
	    float height = 30.0f + noise_1(i / 17.4f) * 15.0f + noise_1(i / 6.3f) * 5.0f;
	    float dirt_layer = 40.0f + noise_1(i / 19.1f) * 15.0f;
	    
	    int mirror_x = 390 - (((390 - i) < 0) ? (i - 390) : (390 - i));
	    
	    height = lerp_smooth((height / 5.0f) + 65.0f, height, (mirror_x / 45.0f) - 1.0f);
	    dirt_layer = lerp_smooth(20.0f, dirt_layer, (mirror_x / 45.0f) - 1.0f);
	    
	    for (int j = 0; j < game.WORLD_HEIGHT; j++) {
	      enum Tiles tile = TILE_STONE;
	      
	      setTile(i, j, TILE_NOTHING);
	      
	      int biome_x = noise_1(j / 21.1f) * 19.0f - 9.5f;
	      
	      fast_srand(i + 1048577 * j);
	      
	      if (mirror_x < 90 + fast_rand() % 4 && j >= height && j < height + 10 + fast_rand() % 4) {
	        setTile(i, j, TILE_SAND);
	        continue;
	      }
	      
	      if (j < height && !(mirror_x > 90 && noise_1(i / 13.4f) > 0.7 && j >= height - 12 && j < height - 8)) continue;
	      
	      if (j < height + dirt_layer) {
	        if (i >= (sand_x + biome_x) - sand_width / 2 && i <= (sand_x + biome_x) + sand_width / 2) {
	          setTile(i, j, TILE_SAND);
	          continue;
	        } else {
	          tile = TILE_DIRT;
	        }
	      }
	      
	      if (!(mirror_x < 90 && j < height + 15) && noise_2(i / 8.0f, j / 6.0f) > 0.65f + lerp(0.15f, 0.0f, (j - height) / dirt_layer)) continue;
	      
	      if (i >= (jungle_x + biome_x) - (jungle_width / 2) && i <= (jungle_x + biome_x) + jungle_width / 2) {
	        if (i < ((jungle_x + biome_x) - (jungle_width / 2) + 20)) {
	          fast_srand(i + 1048577 * j);
	          
	          if (fast_rand() % 20 < i - ((jungle_x + biome_x) - (jungle_width / 2))) {
	            tile = TILE_MUD;
	          }
	        } else if (i > ((jungle_x + biome_x + (jungle_width / 2)) - 20)) {
	          fast_srand(i + 1048577 * j);
	          
	          if (fast_rand() % 20 > i - ((jungle_x + biome_x + (jungle_width / 2)) - 20)) {
	            tile = TILE_MUD;
	          }
	        } else {
	          tile = TILE_MUD;
	        }
	      } else if (j < (game.WORLD_HEIGHT - 125) + (noise_1(i / 7.8f) * 10.0f) && i >= (snow_x + biome_x) - snow_width / 2 && i <= (snow_x + biome_x) + snow_width / 2) {
	        tile = (tile == TILE_DIRT) ? TILE_DIRT /*TILE_SNOW*/ : TILE_STONE /*TILE_ICE*/;
	      }

	      setTile(i, j, tile);
	    }
	  }

	  for (int i = 0; i < 75; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Rocks in dirt(%d/75)", i);
	  	middleText(buffer, updateProgress());
	  	
	    int x = fast_rand() % game.WORLD_WIDTH;
	    int y = fast_rand() % 100;
	    
	    if (getTile(x, y).id != TILE_DIRT && getTile(x, y).id != TILE_MUD) {
	      i--;
	      continue;
	    }
	    
	    clump(x, y, 20, TILE_STONE, 0, 1, 1.3f, 4.7f);
	  }
	  
	  for (int i = 0; i < 75; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Clay(%d/75)", i);
	  	middleText(buffer, updateProgress());
	  		  	
	    int x = fast_rand() % game.WORLD_WIDTH;
	    int y = fast_rand() % 100;
	    
	    if (getTile(x, y).id != TILE_DIRT) {
	      i--;
	      continue;
	    }
	    
	    clump(x, y, 20, TILE_CLAY, 0, 1, 7.9f, 1.2f);
	  }
	  
	  for (int i = 0; i < 30; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Sand(%d/30)", i);
	  	middleText(buffer, updateProgress());
	    
	    int x = fast_rand() % game.WORLD_WIDTH;
	    int y = (fast_rand() % 20) + 100;
	    
	    if (getTile(x, y).id != TILE_DIRT && getTile(x, y).id != TILE_STONE) {
	      i--;
	      continue;
	    }
	    
	    clump(x, y, 20, TILE_SAND, 0, 1, 31.4f, 15.9f);
	  }
	  
	  for (int i = 0; i < game.WORLD_HEIGHT; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Grass(%d/%d)", i, game.WORLD_HEIGHT);
	  	middleText(buffer, updateProgress());
	    
	    for (int j = 0; j < game.WORLD_WIDTH; j++) {
	      enum Tiles tile;
	      
	      if (getTile(j, i).id == TILE_DIRT) {
	        tile = TILE_GRASS;
	      } else if (getTile(j, i).id == TILE_MUD) {
	        tile = TILE_GRASS_JUNGLE;
	      } else {
	        continue;
	      }
	      
	      if (j > 0) {
	        if (getTile(j - 1, i).id == TILE_NOTHING) {
			  setTile(j, i, tile);
	          continue;
	        }
	      }
	      
	      if (i > 0) {
	        if (getTile(j, i - 1).id == TILE_NOTHING) {
	          setTile(j, i, tile);
	          continue;
	        }
	      }
	      
	      if (j < game.WORLD_WIDTH - 1) {
	        if (getTile(j + 1, i).id == TILE_NOTHING) {
	          setTile(j, i, tile);
	          continue;
	        }
	      }
	      
	      if (i < game.WORLD_HEIGHT - 1) {
	        if (getTile(j, i + 1).id == TILE_NOTHING) {
	          setTile(j, i, tile);
	          continue;
	        }
	      }
	    }
	  }

	  /*
	  
	  for (int i = 0; i < 300; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Dirt in rocks(%d/300)", i);
	  	middleText(buffer, updateProgress());
	    
	    int x = fast_rand() % game.WORLD_WIDTH;
	    int y = fast_rand() % game.WORLD_HEIGHT;
	    
	    if (getTile(x, y).id != TILE_STONE) {
	      i--;
	      continue;
	    }
	    
	    clump(x, y, 15, TILE_DIRT, 0, 1, 9.1f, 5.6f);
	  }
	  
	  for (int i = 0; i < game.WORLD_WIDTH; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Gravitating sand(%d/%d)", i, game.WORLD_WIDTH);
	  	middleText(buffer, updateProgress());
	    
	    for (int j = game.WORLD_HEIGHT - 2; j >= 0; j--) {
	      if (getTile(i, j).id != TILE_SAND) continue;
	      int y = j;
	      
	      while (y < game.WORLD_HEIGHT - 1 && getTile(i, y + 1).id == TILE_NOTHING) {
	        y++;
	      }
	      
	      setTile(i, j, TILE_NOTHING);
	      setTile(i, y, TILE_SAND);
	    }
	  }
	  
	  for (int i = 0; i < 150; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Water(%d/150)", i);
	  	middleText(buffer, updateProgress());
	    
	    int x = fast_rand() % game.WORLD_WIDTH;
	    int y = (fast_rand() % (game.WORLD_HEIGHT - 125)) + 50;
	    
	    if (getTile(x, y).id != TILE_NOTHING) {
	      i--;
	      continue;
	    }
	    
	    int area = flood(x, y, TILE_WATER);
	    
	    if (area > (fast_rand() % 150) + 100) {
	      unflood(x, y, TILE_WATER);
	      i--;
	    }
	  }
	  
	  middleText("Beaches", updateProgress());
	  
	  flood(0, 50, TILE_WATER);
	  flood(game.WORLD_WIDTH - 1, 50, TILE_WATER);
	  
	  for (int i = 0; i < game.WORLD_WIDTH; i++) {
	  	char buffer[30];
	  	sprintf(buffer, "Vines(%d/%d)", i, game.WORLD_WIDTH);
	  	middleText(buffer, updateProgress());
	    
	    int y = 0;
	    int length = 0;
	    
	    for (int j = 0; j < game.WORLD_HEIGHT; j++) {
	      if (getTile(i, j).id == TILE_GRASS && fast_rand() % 3 == 0) {
	        y = j;
	        length = (fast_rand() % 8) + 3;
	      } else if (getTile(i, j).id == TILE_GRASS_JUNGLE && fast_rand() % 2 == 0) {
	        y = j;
	        length = (fast_rand() % 8) + 3;
	      } else if (getTile(i, j).id == TILE_NOTHING) {
	        if (j > y && j <= y + length) {
	          setTile(i, j, TILE_VINE);
	        }
	      } else {
	        length = 0;
	      }
	    }
	  }
	  
	  middleText("Trees", updateProgress());
	  
	  for (int i = (fast_rand() % 5) + 5; i < game.WORLD_WIDTH; i += (fast_rand() % 5) + 5) {
	    for (int j = 0; j < game.WORLD_HEIGHT - 1; j++) {
	      if (getTile(i, j).id == TILE_NOTHING && (getTile(i, j + 1).id == TILE_GRASS || getTile(i, j + 1).id == TILE_GRASS_JUNGLE)) {
	        int length = (getTile(i, j + 1).id == TILE_GRASS_JUNGLE ? 5 : 1) + (fast_rand() % 7);

			generateTree(i, j, length);
	        break;
	      }
	    }
	  }
	*/

	/*
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

//	Weeds
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
	*/
}
