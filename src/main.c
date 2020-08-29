#include <gint/std/stdlib.h>
#include <gint/defs/util.h>
#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <stdbool.h>
#include <gint/std/string.h>
#include <gint/clock.h>

#include "defs.h"
#include "syscalls.h"
#include "world.h"
#include "entity.h"
#include "render.h"
#include "menu.h"
#include "save.h"
#include "crafting.h"

enum UpdateReturnCodes {
	UPDATE_EXIT, 		// Exit the game
	UPDATE_CONTINUE,	// Continue as normal
	UPDATE_AGAIN		// Run update again ASAP
};

// Syscalls
const unsigned int sc003B[] = { SCA, SCB, SCE, 0x003B };
const unsigned int sc019F[] = { SCA, SCB, SCE, 0x019F };
const unsigned int sc0236[] = { SCA, SCB, SCE, 0x0236 };
const unsigned int sc042E[] = { SCA, SCB, SCE, 0x042E };

// Global variables
struct SaveData save;
struct World world;
struct Player player;

// Governs the 60UPS loop
int frameCallback(volatile int *flag)
{
	*flag = 1;
	return TIMER_CONTINUE;
}

// Checks if the RAM we'll be using to store world tiles works
bool testRAM()
{
	unsigned int *RAMAddress = (void*)RAM_START;
	unsigned int *RAMTestAddress = (void*)0x88000000;
	*RAMAddress = 0xC0FFEE;
	if(*RAMAddress == 0xC0FFEE && *RAMAddress != *RAMTestAddress) return true;
	return false;
}

void positionPlayerAtWorldMiddle()
{
	int playerX = (WORLD_WIDTH / 2);
	int playerY = 0;
	while(1)
	{
		if(getTile(playerX, playerY).idx != 0)
		{
			playerY = (playerY - 3);
			break;
		}
		playerY++;
	}

	player.props.x = playerX << 3;
	player.props.y = playerY << 3;
}

// Update player and do keyboard stuff
enum UpdateReturnCodes keyboardUpdate()
{
	bool validLeft, validRight, validTop, validBottom;
	int x, y;
	key_event_t key;
	enum Tiles tile;
	bool playerDead =  player.combat.health <= 0;

	key = pollevent();
	while(key.type != KEYEV_NONE)
	{
		switch(key.key)
		{
			case KEY_OPTN:
				if(key.type == KEYEV_DOWN) gint_switch(&takeVRAMCapture);
				break;
			
			case KEY_SHIFT:
				if(key.type != KEYEV_DOWN || playerDead) break;
				inventoryMenu();
//				Immediately go to crafting
				if(keydown(KEY_ALPHA))
				{
					key.key = KEY_ALPHA;
					continue;
				}
				break;
			case KEY_ALPHA:
				if(key.type != KEYEV_DOWN || playerDead) break;
				craftingMenu();
//				Immediately go to inventory
				if(keydown(KEY_SHIFT))
				{
					key.key = KEY_SHIFT;
					continue;
				}
				break;
			
			case KEY_F1: case KEY_F2: case KEY_F3: case KEY_F4: case KEY_F5:
				if(player.swingFrame == 0 && !playerDead) player.inventory.hotbarSlot = keycode_function(key.key) - 1;
				break;

			case KEY_MENU:
				if(exitMenu()) return UPDATE_EXIT;
				break;
			
			default:
				break;
		}
		key = pollevent();
	}

//	These need to run as long as the button is held

	if(!playerDead)
	{
//		Remove tile
		if(keydown(KEY_7) && player.swingFrame == 0)
		{
			if(items[player.inventory.getSelected()->id].canSwing)
			{
				player.swingFrame = 32;
				player.swingDir = player.cursorTile.x < player.props.x >> 3;
			}
			else
			{
				x = player.cursorTile.x;
				y = player.cursorTile.y;
				world.removeTile(x, y);
			}
		}

//		Place tile
		if(keydown(KEY_9))
		{
			x = player.cursorTile.x;
			y = player.cursorTile.y;
			validLeft = x < player.props.x >> 3;
			validRight = x > (player.props.x + player.props.width) >> 3;
			validTop = y < player.props.y >> 3;
			validBottom = y > (player.props.y + player.props.height) >> 3;
			tile = items[player.inventory.getSelected()->id].tile;
			if(tile != TILE_NULL && tile != TILE_NOTHING)
			{
				if(validLeft || validRight || validTop || validBottom || tiles[tile].physics != PHYS_SOLID)
				{
					world.placeTile(x, y, player.inventory.getSelected());
				}
			}
		}

//		Movement
		if(keydown(KEY_4) && player.props.xVel > -1) player.props.xVel -= 0.3;
		if(keydown(KEY_6) && player.props.xVel < 1) player.props.xVel += 0.3;
		if(keydown(KEY_8) && player.props.touchingTileTop) 
		{
			player.props.yVel = -4.5;
			player.props.dropping = true;
		}
		if(keydown(KEY_2)) player.props.dropping = true;
		if((!keydown(KEY_8) && !keydown(KEY_2)) || (keydown(KEY_8) && !keydown(KEY_2) && player.props.yVel <= 0)) player.props.dropping = false;

//		Cursor
		if(keydown(KEY_LEFT)) player.cursor.x--;
		if(keydown(KEY_RIGHT)) player.cursor.x++;
		if(keydown(KEY_UP)) player.cursor.y--;
		if(keydown(KEY_DOWN)) player.cursor.y++;
		player.cursor.x = min(max(0, player.cursor.x), SCREEN_WIDTH - 1);
		player.cursor.y = min(max(0, player.cursor.y), SCREEN_HEIGHT - 1);
	}

	return UPDATE_CONTINUE;
}

void playerUpdate(int frames)
{
	struct AnimationData *anim = &player.anim;
	int animFrames[][2] = {
		{0, 0},
		{1, 4},
		{5, 5},
		{6, 19}
	};

//	Handle the physics for the player
	player.physics(&player.props);

//	Figure out which animation frame the player should be in right now
	if(player.props.xVel > 0)
	{
		anim->direction = 0;
	}
	else if(player.props.xVel < 0)
	{
		anim->direction = 1;
	}

	if(!player.props.touchingTileTop)
	{
		anim->animation = 2;
		anim->animationFrame = 5;
	}
	else if(player.props.xVel != 0 && anim->animation != 3)
	{
		anim->animation = 3;
		anim->animationFrame = 6;
	}
	else if(player.props.xVel == 0)
	{
		anim->animation = 0;
		anim->animationFrame = 0;
	}
	else 
	{
		if(frames & 1) anim->animationFrame++;
	}

	if(anim->animationFrame > animFrames[anim->animation][1]) 
	{
		anim->animationFrame = animFrames[anim->animation][0];
	}
}

// frames increases 60 times per second, take into account
int main(void)
{
	int menuSelect;
	extern bopti_image_t img_generating;
	extern font_t font_smalltext;
	bool renderThisFrame = true;
	int w, h;
	int timer;
	enum UpdateReturnCodes updateRet;
	volatile int flag = 0;
	int frames = 0;
	int mediaFree[2];
	int respawnCounter = 0;

	save = (struct SaveData){
		.tileDataSize = WORLD_HEIGHT * WORLD_WIDTH * sizeof(Tile),
		.regionsX = REGIONS_X,
		.regionsY = REGIONS_Y,
		.tileData = (void*)RAM_START,
		.regionData = { 0 },
		.error = -99,
	};

//	Makes it easier to see if a world load error occurs
	memset(save.tileData, 0, WORLD_WIDTH * WORLD_HEIGHT * sizeof(Tile));

	if(!testRAM()) 
	{
		RAMErrorMenu();
		return 1;
	}

	srand(RTC_GetTicks());

	dgray_setdelays(923, 1742);
	dgray(DGRAY_ON);

	menuSelect = mainMenu();
	if(menuSelect == -1) return 1;
	
	player = (struct Player){

		.props = {
			.width = 12, 
			.height = 21, 
			.x = 0, 
			.y = 0, 
			.xVel = 0.0, 
			.yVel = 0.0, 
			.touchingTileTop = false, 
			.dropping = false
		},

		.combat = {
			.health = 100,
			.alignment = ALIGN_NEUTRAL,
			.immuneFrames = 40,
			.currImmuneFrames = 0,
			.attack = 0,
			.defense = 0,
			.knockbackResist = 0.0
		},

		.cursor = {75, 32},
		.cursorTile = { 0 },

		.anim = { 0 },

		.inventory = {
			{{ 0 }},
			0,
			.getFirstFreeSlot = &getFirstFreeSlot,
			.removeItem = &removeItem,
			.stackItem = &stackItem,
			.tallyItem = &tallyItem,
			.findSlot = &findSlot,
			.getSelected = &getSelected
		},

		.physics = &handlePhysics
	};

	for(int slot = 0; slot < INVENTORY_SIZE; slot++) player.inventory.items[slot] = (Item){ITEM_NULL, 0};

	world = (struct World)
	{
		.tiles = (Tile*)save.tileData,
		.entities = (Entity*)malloc(MAX_ENTITIES * sizeof(Entity)),
		.explosion = {
			.numParticles = 0,
			.particles = malloc(50 * sizeof(Particle)),
			.deltaTicks = 0
		 },

		.placeTile = &placeTile,
		.removeTile = &removeTile,
		
		.spawnEntity = &spawnEntity,
		.removeEntity = &removeEntity,
		.checkFreeEntitySlot = &checkFreeEntitySlot
	};

//	An entity ID of -1 is considered a free slot
	for(int idx = 0; idx < MAX_ENTITIES; idx++) world.entities[idx] = (Entity){ -1 };

	if(menuSelect == 0) // New game
	{
		dclear(C_WHITE);
		dimage(31, 15, &img_generating);
		dupdate();
		generateWorld();
		memset(save.regionData, 1, save.regionsX * save.regionsY);
		player.inventory.items[0] = (Item){ITEM_SWORD, 1};
	} 
	else if(menuSelect == 1) // Load game
	{
		dgray(DGRAY_OFF);
		dclear(C_WHITE);
		dsize("Loading World...", NULL, &w, &h);
		dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Loading World...");
		dupdate();
		gint_switch(&loadSave);
		if(save.error != -99) 
		{
			loadFailMenu();
			return 1;
		}

		dgray(DGRAY_ON);
	}

	dfont(&font_smalltext);

	positionPlayerAtWorldMiddle();

	timer = timer_setup(TIMER_ANY, (1000 / 60) * 1000, &frameCallback, &flag);
	timer_start(timer);

	//world.spawnEntity(ENT_SLIME, player.props.x, player.props.y - 100);

	while(true)
	{
		if(!respawnCounter) playerUpdate(frames);

		updateRet = keyboardUpdate();
		if(updateRet == UPDATE_EXIT) break;
		else if(updateRet == UPDATE_AGAIN) continue;

		doEntityCycle(frames);
		doSpawningCycle();
		if(player.combat.health <= 0)
		{
			if(respawnCounter == 1)
			{
				positionPlayerAtWorldMiddle();
				player.combat.health = 100;
				player.props.xVel = 0;
				player.props.yVel = 0;
				player.props.dropping = false;
				respawnCounter = 0;
				player.combat.currImmuneFrames = player.combat.immuneFrames;
			}
			else if(respawnCounter > 0) respawnCounter--;
			else
			{
				createExplosion(&world.explosion, player.props.x + (player.props.width >> 1), player.props.y + (player.props.height >> 1));
				respawnCounter = 300;
			}
		}
		if(renderThisFrame)
		{
			render();
			dupdate();
			renderThisFrame = false;
		}
		else renderThisFrame = true;

		frames++;
		while(!flag) sleep();
		flag = 0;
	}

	timer_stop(timer);
	dgray(DGRAY_OFF);
	dclear(C_WHITE);
	dfont(NULL);
	dsize("Saving World...", NULL, &w, &h);
	dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Saving World...");
	dupdate();

	free(world.entities);
	destroyExplosion(&world.explosion);

	gint_switch(&saveGame);
	if(save.error != -99) saveFailMenu();
	
	Bfile_GetMediaFree_OS(u"\\\\fls0", mediaFree);
	if(mediaFree[1] < 250000) lowSpaceMenu(mediaFree[1]);
	
	//SMEM_Optimization();
	RebootOS();

	return 1;
}
