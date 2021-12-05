#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define WORLD_WIDTH  780
#define WORLD_HEIGHT 320

typedef enum tile_t tile_t;

enum tile_t {
  tile_air,
  tile_dirt,
  tile_grass,
  tile_mud,
  tile_jungle_grass,
  tile_stone,
  tile_snow,
  tile_ice,
  tile_sand,
  tile_clay,
  tile_copper,
  tile_tin,
  tile_iron,
  tile_lead,
  tile_gold,
  tile_platinum,
  tile_silver,
  tile_tungsten,
  tile_vine,
  tile_water,
  tile_lava,
  tile_ash,
  tile_hellstone,
  tile_tree,
  tile_wood,
  tile_life_crystal,
  tile_chest,
  tile_cactus,
  tile_cobweb
};

tile_t *world;

const Color colors[] = {
  (Color){132, 170, 248, 255},
  (Color){151, 107, 75, 255},
  (Color){28, 216, 94, 255},
  (Color){92, 68, 73, 255},
  (Color){143, 215, 29, 255},
  (Color){128, 128, 128, 255},
  (Color){223, 223, 223, 255},
  (Color){159, 191, 255, 255},
  (Color){211, 198, 111, 255},
  (Color){146, 81, 68, 255},
  (Color){150, 67, 22, 255},
  (Color){129, 125, 93, 255},
  (Color){181, 164, 149, 255},
  (Color){43, 60, 86, 255},    // color chosen at random, not from map
  (Color){173, 145, 40, 255},  // ^^^
  (Color){143, 142, 162, 255}, // ^^^
  (Color){158, 163, 176, 255}, // ^^^
  (Color){90, 125, 83, 255},   // ^^^
  (Color){30, 150, 72, 255},
  (Color){9, 61, 191, 255},
  (Color){255, 95, 15, 255},
  (Color){47, 47, 47, 255},
  (Color){142, 66, 66, 255},
  (Color){151, 107, 75, 255},
  (Color){169, 125, 93, 255},
  (Color){182, 18, 57, 255},
  (Color){161, 134, 160, 255},
  (Color){73, 120, 17, 255},
  (Color){158, 173, 174, 255}
};

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
  srand(x + 1048577 * y);
  return (rand() % 65537) / 65537.0f;
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

int clump(int x, int y, int radius, tile_t tile, int invert, int mask_empty, float off_x, float off_y) {
  if (mask_empty && (invert ? (world[x + y * WORLD_WIDTH] != tile_air) : (world[x + y * WORLD_WIDTH] == tile_air))) return 0;
  
  for (int dx = x - radius / 2; dx <= x + radius / 2; dx++) {
    if (dx >= WORLD_WIDTH) break;
    if (dx < 0) continue;
    
    for (int dy = y - radius / 2; dy <= y + radius / 2; dy++) {
      if (dy >= WORLD_WIDTH) break;
      if (dy < 0) continue;
      
      int dist = (dx - x) * (dx - x) + (dy - y) * (dy - y);
      
      if (dist > radius) {
        if (dy >= y) break;
        else continue;
      }
      
      if (mask_empty && (invert ? (world[dx + dy * WORLD_WIDTH] != tile_air) : (world[dx + dy * WORLD_WIDTH] == tile_air))) continue;
      if (noise_2(dx / 3.0f + off_x, dy / 3.0f + off_y) < 0.5f) continue;
      
      world[dx + dy * WORLD_WIDTH] = tile;
    }
  }
  
  return 1;
}

int unflood(int x, int y, tile_t tile) {
  if (x < 0 || y < 0 || x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return 0;
  if (world[x + y * WORLD_WIDTH] != tile) return 0;
  
  world[x + y * WORLD_WIDTH] = tile_air;
  
  return unflood(x - 1, y, tile) + unflood(x + 1, y, tile) + unflood(x, y + 1, tile) + 1;
}

int flood(int x, int y, tile_t tile) {
  if (x < 0 || y < 0 || x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return 0;
  if (world[x + y * WORLD_WIDTH] != tile_air) return 0;
  
  world[x + y * WORLD_WIDTH] = tile;
  
  return flood(x - 1, y, tile) + flood(x + 1, y, tile) + flood(x, y + 1, tile) + 1;
}

int main(void) {
  InitWindow(WORLD_WIDTH * 2, WORLD_HEIGHT * 2, "world!");
  world = malloc(WORLD_WIDTH * WORLD_HEIGHT * sizeof(tile_t));
  
  srand(0); // TODO
  
  printf("Terrain\n");
  
  int sand_x = rand() % 4;
  if (sand_x >= 2) sand_x++;
  
  int snow_x = 4 - sand_x;
  int jungle_x = rand() % 5;
  
  while (jungle_x == sand_x || jungle_x == snow_x) snow_x = rand() % 5;
  
  sand_x = 90 + ((sand_x * 2 + 1) * (WORLD_WIDTH - 180)) / 10;
  snow_x = 90 + ((snow_x * 2 + 1) * (WORLD_WIDTH - 180)) / 10;
  jungle_x = 90 + ((jungle_x * 2 + 1) * (WORLD_WIDTH - 180)) / 10;
  
  int sand_width = ((WORLD_WIDTH - 180) / 5) - 25 - (rand() % 10);
  int snow_width = ((WORLD_WIDTH - 180) / 5) - 35 - (rand() % 10);
  int jungle_width = ((WORLD_WIDTH - 180) / 5) - (rand() % 10);
  
  for (int i = 0; i < WORLD_WIDTH; i++) {
    float height = 30.0f + noise_1(i / 17.4f) * 15.0f + noise_1(i / 6.3f) * 5.0f;
    float dirt_layer = 40.0f + noise_1(i / 19.1f) * 15.0f;
    
    int mirror_x = 390 - (((390 - i) < 0) ? (i - 390) : (390 - i));
    
    height = lerp_smooth((height / 5.0f) + 65.0f, height, (mirror_x / 45.0f) - 1.0f);
    dirt_layer = lerp_smooth(20.0f, dirt_layer, (mirror_x / 45.0f) - 1.0f);
    
    for (int j = 0; j < WORLD_HEIGHT; j++) {
      tile_t tile = tile_stone;
      
      world[i + j * WORLD_WIDTH] = tile_air;
      
      int biome_x = noise_1(j / 21.1f) * 19.0f - 9.5f;
      
      srand(i + 1048577 * j);
      
      if (mirror_x < 90 + rand() % 4 && j >= height && j < height + 10 + rand() % 4) {
        world[i + j * WORLD_WIDTH] = tile_sand;
        continue;
      }
      
      if (j < height && !(mirror_x > 90 && noise_1(i / 13.4f) > 0.7 && j >= height - 12 && j < height - 8)) continue;
      
      if (j < height + dirt_layer) {
        if (i >= (sand_x + biome_x) - sand_width / 2 && i <= (sand_x + biome_x) + sand_width / 2) {
          world[i + j * WORLD_WIDTH] = tile_sand;
          continue;
        } else {
          tile = tile_dirt;
        }
      }
      
      if (!(mirror_x < 90 && j < height + 15) && noise_2(i / 8.0f, j / 6.0f) > 0.65f + lerp(0.15f, 0.0f, (j - height) / dirt_layer)) continue;
      
      if (i >= (jungle_x + biome_x) - (jungle_width / 2) && i <= (jungle_x + biome_x) + jungle_width / 2) {
        if (i < ((jungle_x + biome_x) - (jungle_width / 2) + 20)) {
          srand(i + 1048577 * j);
          
          if (rand() % 20 < i - ((jungle_x + biome_x) - (jungle_width / 2))) {
            tile = tile_mud;
          }
        } else if (i > ((jungle_x + biome_x + (jungle_width / 2)) - 20)) {
          srand(i + 1048577 * j);
          
          if (rand() % 20 > i - ((jungle_x + biome_x + (jungle_width / 2)) - 20)) {
            tile = tile_mud;
          }
        } else {
          tile = tile_mud;
        }
      } else if (j < (WORLD_HEIGHT - 125) + (noise_1(i / 7.8f) * 10.0f) && i >= (snow_x + biome_x) - snow_width / 2 && i <= (snow_x + biome_x) + snow_width / 2) {
        tile = (tile == tile_dirt) ? tile_snow : tile_ice;
      }
      
      world[i + j * WORLD_WIDTH] = tile;
    }
  }
  
  printf("Rocks in dirt\n");
  
  for (int i = 0; i < 75; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % 100;
    
    if (world[x + y * WORLD_WIDTH] != tile_dirt && world[x + y * WORLD_WIDTH] != tile_mud) {
      i--;
      continue;
    }
    
    clump(x, y, 20, tile_stone, 0, 1, 1.3f, 4.7f);
  }
  
  printf("Clay\n");
  
  for (int i = 0; i < 75; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % 100;
    
    if (world[x + y * WORLD_WIDTH] != tile_dirt) {
      i--;
      continue;
    }
    
    clump(x, y, 20, tile_clay, 0, 1, 7.9f, 1.2f);
  }
  
  printf("Sand\n");
  
  for (int i = 0; i < 30; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % 20) + 100;
    
    if (world[x + y * WORLD_WIDTH] != tile_dirt && world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 20, tile_sand, 0, 1, 31.4f, 15.9f);
  }
  
  printf("Grass\n");
  
  for (int i = 0; i < WORLD_HEIGHT; i++) {
    for (int j = 0; j < WORLD_WIDTH; j++) {
      tile_t tile;
      
      if (world[j + i * WORLD_WIDTH] == tile_dirt) {
        tile = tile_grass;
      } else if (world[j + i * WORLD_WIDTH] == tile_mud) {
        tile = tile_jungle_grass;
      } else {
        continue;
      }
      
      if (j > 0) {
        if (world[(j - 1) + i * WORLD_WIDTH] == tile_air) {
          world[j + i * WORLD_WIDTH] = tile;
          continue;
        }
      }
      
      if (i > 0) {
        if (world[j + (i - 1) * WORLD_WIDTH] == tile_air) {
          world[j + i * WORLD_WIDTH] = tile;
          continue;
        }
      }
      
      if (j < WORLD_WIDTH - 1) {
        if (world[(j + 1) + i * WORLD_WIDTH] == tile_air) {
          world[j + i * WORLD_WIDTH] = tile;
          continue;
        }
      }
      
      if (i < WORLD_HEIGHT - 1) {
        if (world[j + (i + 1) * WORLD_WIDTH] == tile_air) {
          world[j + i * WORLD_WIDTH] = tile;
          continue;
        }
      }
    }
  }
  
  printf("Dirt in rocks\n");
  
  for (int i = 0; i < 300; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % WORLD_HEIGHT;
    
    if (world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 15, tile_dirt, 0, 1, 9.1f, 5.6f);
  }
  
  printf("Shinies\n");
  
  tile_t ore_1 = (rand() % 2) ? tile_copper : tile_tin;      // 200, 20
  tile_t ore_2 = (rand() % 2) ? tile_iron : tile_lead;       // 175, 18
  tile_t ore_3 = (rand() % 2) ? tile_gold : tile_platinum;   // 125, 14
  tile_t ore_4 = (rand() % 2) ? tile_silver : tile_tungsten; // 150, 16
  
  for (int i = 0; i < 200; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % WORLD_HEIGHT;
    
    if (world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 20, ore_1, 0, 1, 13.6f, 18.9f);
  }
  
  for (int i = 0; i < 175; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % WORLD_HEIGHT;
    
    if (world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 18, ore_2, 0, 1, 14.6f, 17.9f);
  }
  
  for (int i = 0; i < 150; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % WORLD_HEIGHT;
    
    if (world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 14, ore_3, 0, 1, 15.6f, 16.9f);
  }
  
  for (int i = 0; i < 125; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % WORLD_HEIGHT;
    
    if (world[x + y * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    clump(x, y, 16, ore_4, 0, 1, 16.6f, 15.9f);
  }
  
  printf("Gravitating sand\n");
  
  for (int i = 0; i < WORLD_WIDTH; i++) {
    for (int j = WORLD_HEIGHT - 2; j >= 0; j--) {
      if (world[i + j * WORLD_WIDTH] != tile_sand) continue;
      int y = j;
      
      while (y < WORLD_HEIGHT - 1 && world[i + (y + 1) * WORLD_WIDTH] == tile_air) {
        y++;
      }
      
      world[i + j * WORLD_WIDTH] = tile_air;
      world[i + y * WORLD_WIDTH] = tile_sand;
    }
  }
  
  printf("Water\n");
  
  for (int i = 0; i < 150; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % (WORLD_HEIGHT - 125)) + 50;
    
    if (world[x + y * WORLD_WIDTH] != tile_air) {
      i--;
      continue;
    }
    
    int area = flood(x, y, tile_water);
    
    if (area > (rand() % 150) + 100) {
      unflood(x, y, tile_water);
      i--;
    }
  }
  
  printf("Beaches\n");
  
  flood(0, 50, tile_water);
  flood(WORLD_WIDTH - 1, 50, tile_water);
  
  printf("Lava\n");
  
  for (int i = 0; i < 50; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % 50) + (WORLD_HEIGHT - 100);
    
    if (world[x + y * WORLD_WIDTH] != tile_air) {
      i--;
      continue;
    }
    
    int area = flood(x, y, tile_lava);
    
    if (area > (rand() % 150) + 100) {
      unflood(x, y, tile_lava);
      i--;
    }
  }
  
  printf("Cobweb\n");
  
  for (int i = 0; i < 50; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % (WORLD_HEIGHT - 155)) + 105;
    
    if (world[x + y * WORLD_WIDTH] != tile_air) {
      i--;
      continue;
    }
    
    clump(x, y, 35, tile_cobweb, 1, 1, 7.7f, 6.6f);
  }
  
  printf("Rooms\n");
  
  int chests[100];
  int chest_count = 0;
  
  for (int i = 0; i < 15; i++) {
    int width = 10 + (rand() % 11);
    int height = 8;
    
    int x = rand() % (WORLD_WIDTH - width);
    int y = (rand() % (WORLD_HEIGHT - (height + 175))) + 100;
    
    if (world[x + y * WORLD_WIDTH] != tile_air) {
      i--;
      continue;
    }
    
    int valid = 1;
    int chest_placed = 0;
      
    for (int j = 0; j < width; j++) {
      for (int k = 0; k < height; k++) {
        if (world[(x + j) + (y + k) * WORLD_WIDTH] == tile_wood) {
          valid = 0;
        }
      }
    }
    
    if (!valid) {
      i--;
      continue;
    }
    
    for (;;) {
      for (int j = 0; j < width; j++) {
        for (int k = 0; k < height; k++) {
          tile_t tile = tile_air;
          
          if (j == 0 || k == 0 || j == width - 1 || k == height - 1) {
            tile = tile_wood;
          }
          
          world[(x + j) + (y + k) * WORLD_WIDTH] = tile;
        }
      }
      
      if (!chest_placed && rand() % 3 == 0) {
        int chest_x = (rand() % (width - 3)) + x + 1;
        int chest_y = (height - 3) + y;
        
        chests[2 * chest_count + 0] = chest_x;
        chests[2 * chest_count + 1] = chest_y;
        
        chest_count++;
        
        world[(chest_x + 0) + (chest_y + 0) * WORLD_WIDTH] = tile_chest;
        world[(chest_x + 1) + (chest_y + 0) * WORLD_WIDTH] = tile_chest;
        world[(chest_x + 0) + (chest_y + 1) * WORLD_WIDTH] = tile_chest;
        world[(chest_x + 1) + (chest_y + 1) * WORLD_WIDTH] = tile_chest;
        
        chest_placed = 1;
      }
      
      y += (height - 1);
      
      int new_width = 10 + (rand() % 11);
      
      x += (new_width - width) / 2;
      x += (rand() % (int)(width / 1.5f)) - (width / 3);
      
      if (x < 0) x = 0;
      if (x + new_width > WORLD_WIDTH) x = WORLD_WIDTH - new_width;
      
      width = new_width;
      height = 7 + (rand() % 3);
      
      if (y >= WORLD_HEIGHT - (height + 125)) break;
      if (rand() % 2 > 0) break;
    }
  }
  
  printf("Life Crystals\n");
  
  int crystals[32];
  
  for (int i = 0; i < 16; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % ((WORLD_HEIGHT / 2) - 50)) + (WORLD_HEIGHT / 2);
    
    if (world[(x + 0) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 0) + (y + 1) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 1) * WORLD_WIDTH] != tile_air) {
      i--;
      continue;
    }
    
    int valid = 1;
    
    for (int j = 0; j < i; j++) {
      int dist = (crystals[2 * j + 0] - x) * (crystals[2 * j + 0] - x) + (crystals[2 * j + 1] - y) * (crystals[2 * j + 1] - y);
      
      if (dist < 900) {
        valid = 0;
        break;
      }
    }
    
    if (!valid) {
      i--;
      continue;
    }
    
    world[(x + 0) + (y + 0) * WORLD_WIDTH] = tile_life_crystal;
    world[(x + 1) + (y + 0) * WORLD_WIDTH] = tile_life_crystal;
    world[(x + 0) + (y + 1) * WORLD_WIDTH] = tile_life_crystal;
    world[(x + 1) + (y + 1) * WORLD_WIDTH] = tile_life_crystal;
    
    crystals[2 * i + 0] = x;
    crystals[2 * i + 1] = y;
  }
  
  printf("Underground Chests\n");
  
  for (int i = 0; i < 20; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = (rand() % (WORLD_HEIGHT - 155)) + 105;
    
    if (world[(x + 0) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 0) + (y + 1) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 1) * WORLD_WIDTH] != tile_air ||
        world[(x + 0) + (y + 2) * WORLD_WIDTH] != tile_stone ||
        world[(x + 1) + (y + 2) * WORLD_WIDTH] != tile_stone) {
      i--;
      continue;
    }
    
    int valid = 1;
    
    for (int j = 0; j < chest_count; j++) {
      int dist = (chests[2 * j + 0] - x) * (chests[2 * j + 0] - x) + (chests[2 * j + 1] - y) * (chests[2 * j + 1] - y);
      
      if (dist < 900) {
        valid = 0;
        break;
      }
    }
    
    if (!valid) {
      i--;
      continue;
    }
    
    world[(x + 0) + (y + 0) * WORLD_WIDTH] = tile_chest;
    world[(x + 1) + (y + 0) * WORLD_WIDTH] = tile_chest;
    world[(x + 0) + (y + 1) * WORLD_WIDTH] = tile_chest;
    world[(x + 1) + (y + 1) * WORLD_WIDTH] = tile_chest;
    
    chests[2 * chest_count + 0] = x;
    chests[2 * chest_count + 1] = y;
    
    chest_count++;
  }
  
  printf("Surface Chests\n");
  
  for (int i = 0; i < 15; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = rand() % 65;
    
    if (world[(x + 0) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 0) * WORLD_WIDTH] != tile_air ||
        world[(x + 0) + (y + 1) * WORLD_WIDTH] != tile_air ||
        world[(x + 1) + (y + 1) * WORLD_WIDTH] != tile_air ||
        (world[(x + 0) + (y + 2) * WORLD_WIDTH] != tile_grass && world[(x + 0) + (y + 2) * WORLD_WIDTH] != tile_jungle_grass) ||
        (world[(x + 1) + (y + 2) * WORLD_WIDTH] != tile_grass && world[(x + 1) + (y + 2) * WORLD_WIDTH] != tile_jungle_grass)) {
      i--;
      continue;
    }
    
    int valid = 0;
    
    for (int j = 0; j < y; j++) {
      if (world[x + j * WORLD_WIDTH] != tile_air) {
        valid = 1;
        break;
      }
    }
    
    if (!valid) {
      i--;
      continue;
    }
    
    for (int j = 0; j < chest_count; j++) {
      int dist = (chests[2 * j + 0] - x) * (chests[2 * j + 0] - x) + (chests[2 * j + 1] - y) * (chests[2 * j + 1] - y);
      
      if (dist < 100) {
        valid = 0;
        break;
      }
    }
    
    if (!valid) {
      i--;
      continue;
    }
    
    world[(x + 0) + (y + 0) * WORLD_WIDTH] = tile_chest;
    world[(x + 1) + (y + 0) * WORLD_WIDTH] = tile_chest;
    world[(x + 0) + (y + 1) * WORLD_WIDTH] = tile_chest;
    world[(x + 1) + (y + 1) * WORLD_WIDTH] = tile_chest;
    
    chests[2 * chest_count + 0] = x;
    chests[2 * chest_count + 1] = y;
    
    chest_count++;
  }
  
  printf("Vines\n");
  
  for (int i = 0; i < WORLD_WIDTH; i++) {
    int y = 0;
    int length = 0;
    
    for (int j = 0; j < WORLD_HEIGHT; j++) {
      if (world[i + j * WORLD_WIDTH] == tile_grass && rand() % 3 == 0) {
        y = j;
        length = (rand() % 8) + 3;
      } else if (world[i + j * WORLD_WIDTH] == tile_jungle_grass && rand() % 2 == 0) {
        y = j;
        length = (rand() % 8) + 3;
      } else if (world[i + j * WORLD_WIDTH] == tile_air) {
        if (j > y && j <= y + length) {
          world[i + j * WORLD_WIDTH] = tile_vine;
        }
      } else {
        length = 0;
      }
    }
  }
  
  printf("Trees\n");
  
  for (int i = (rand() % 5) + 5; i < WORLD_WIDTH; i += (rand() % 5) + 5) {
    for (int j = 0; j < WORLD_HEIGHT - 1; j++) {
      if (world[i + j * WORLD_WIDTH] == tile_air && (world[i + (j + 1) * WORLD_WIDTH] == tile_grass || world[i + (j + 1) * WORLD_WIDTH] == tile_jungle_grass)) {
        int length = (world[i + (j + 1) * WORLD_WIDTH] == tile_jungle_grass ? 8 : 5) + (rand() % 4);
        
        for (int k = 0; k < length; k++) {
          world[i + (j - k) * WORLD_WIDTH] = tile_tree;
        }
        
        break;
      }
    }
  }
  
  printf("Cacti\n");
  
  for (int i = (rand() % 5) + 95; i < WORLD_WIDTH - 90; i += (rand() % 10) + 10) {
    for (int j = 0; j < WORLD_HEIGHT - 1; j++) {
      if (world[i + j * WORLD_WIDTH] == tile_air && world[i + (j + 1) * WORLD_WIDTH] == tile_sand) {
        int length = 4 + (rand() % 3);
        
        for (int k = 0; k < length; k++) {
          world[i + (j - k) * WORLD_WIDTH] = tile_cactus;
        }
        
        break;
      }
    }
  }
  
  printf("Underworld\n");
  
  for (int i = 0; i < WORLD_WIDTH; i++) {
    int ceiling_1 = (int)(noise_1(i / 7.1f) * 5.0f + noise_1(i / 3.5f) * 5.0f);
    int ceiling_2 = (int)(noise_1(i / 6.4f) * 5.0f + noise_1(i / 3.3f) * 5.0f);
    int floor = (int)(lerp_smooth(0, 35, noise_1(i / 17.1f) * 0.2f + noise_1(i / 13.5f) * 0.8f));
    
    for (int j = WORLD_HEIGHT - 60; j < WORLD_HEIGHT; j++) {
      if (j < WORLD_HEIGHT - 50) {
        if (j > (WORLD_HEIGHT - 50) - ceiling_1) {
          world[i + j * WORLD_WIDTH] = tile_ash;
        }
      } else {
        if (j > (WORLD_HEIGHT - 50) + ceiling_2 && j < WORLD_HEIGHT - floor) {
          if (j > (WORLD_HEIGHT - 25)) {
            world[i + j * WORLD_WIDTH] = tile_lava;
          } else {
            world[i + j * WORLD_WIDTH] = tile_air;
          }
        } else {
          world[i + j * WORLD_WIDTH] = tile_ash;
        }
      }
    }
  }
  
  printf("Hellstone\n");
  
  for (int i = 0; i < 50; i++) {
    int x = rand() % WORLD_WIDTH;
    int y = WORLD_HEIGHT - (rand() % 35);
    
    if (world[x + y * WORLD_WIDTH] != tile_ash) {
      i--;
      continue;
    }
    
    clump(x, y, 16, tile_hellstone, 0, 1, 16.6f, 15.9f);
  }
  
  printf("Generating image\n");
  Image image = GenImageColor(WORLD_WIDTH * 2, WORLD_HEIGHT * 2, BLACK);
  
  for (int i = 0; i < WORLD_HEIGHT; i++) {
    for (int j = 0; j < WORLD_WIDTH; j++) {
      ImageDrawRectangle(&image, j * 2, i * 2, 2, 2, colors[world[j + i * WORLD_WIDTH]]);
    }
  }
  
  Texture texture = LoadTextureFromImage(image);
  printf("Done!\n");
  
  while (!WindowShouldClose()) {
    BeginDrawing();
    DrawTexture(texture, 0, 0, WHITE);
    EndDrawing();
  }
  
  UnloadTexture(texture);
  UnloadImage(image);
  
  CloseWindow();
}
