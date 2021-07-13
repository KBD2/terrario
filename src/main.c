#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <gint/clock.h>
#include <gint/bfile.h>
#include <gint/hardware.h>
#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/timer.h>

#include "defs.h"
#include "syscalls.h"
#include "world.h"
#include "entity.h"
#include "render.h"
#include "menu.h"
#include "save.h"
#include "crafting.h"
#include "update.h"
#include "generate.h"
#include "optimise.h"
#include "npc.h"

// Fixes implicit declaration warning (feature is not meant for use in normal addins)
void spu_zero();

// Syscalls
const unsigned int sc003B[] = { SCA, SCB, SCE, 0x003B };
const unsigned int sc019F[] = { SCA, SCB, SCE, 0x019F };
const unsigned int sc0236[] = { SCA, SCB, SCE, 0x0236 };
const unsigned int sc042E[] = { SCA, SCB, SCE, 0x042E };
const unsigned int sc015F[] = { SCA, SCB, SCE, 0x015F };

// Global variables
GYRAM char varBuffer[VAR_BUF_HEIGHT][VAR_BUF_WIDTH];
GYRAM struct SaveData save;
struct World world;
struct Player player;
char versionBuffer[16];
struct GameCompatibilityPresets game;

// Governs the 60UPS loop
int frameCallback(volatile int *flag)
{
	*flag = 1;
	return TIMER_CONTINUE;
}

// Checks if the RAM we'll be using to store world tiles works
bool testRAM()
{
	unsigned int *RAMAddress = game.RAM_START;
	unsigned int save = *RAMAddress;
//	Ensures this isn't a 256kB-RAM calc
	if(gint[HWRAM] < 500000) return false;
	*RAMAddress = 0xC0FFEE;
	if(*RAMAddress == 0xC0FFEE)
	{
		*RAMAddress = save;
		return true;
	}
	*RAMAddress = save;
	return false;
}

void setPlayerSpawn()
{
	int playerX = (game.WORLD_WIDTH / 2);
	int playerY = 0;

	while(getTile(playerX, playerY).id == TILE_NOTHING && playerY < game.WORLD_HEIGHT) playerY++;

	player.spawn.x = playerX << 3;
	player.spawn.y = (playerY << 3) - player.props.height;
}

bool gameLoop(volatile int *flag)
{
	int respawnCounter = 0;
	int frames = 0;
	enum UpdateReturnCodes updateRet;
	bool renderThisFrame = true;

	while(true)
	{
		updateRet = keyboardUpdate();
		if(!respawnCounter) playerUpdate(frames);
		if(updateRet == UPDATE_EXIT) return true;
		else if(updateRet == UPDATE_EXIT_NOSAVE) return false;

		if(frames & 1) updateExplosion();
		if(frames % 8 == 0) worldUpdate();
		if(frames % 3600 == 0) doMarkerChecks();

		// Only bother rendering 30 frames (60 updates)
		if(world.explosion.deltaTicks == 30) world.explosion.numParticles = 0;

		doEntityCycle(frames);
		doSpawningCycle();
		npcUpdate(frames);
		if(player.combat.health <= 0)
		{
			if(respawnCounter == 1)
			{
				player.props.x = player.spawn.x;
				player.props.y = player.spawn.y;
				player.combat.health = player.maxHealth;
				player.props.xVel = 0;
				player.props.yVel = 0;
				player.props.dropping = false;
				respawnCounter = 0;
				player.combat.currImmuneFrames = player.combat.immuneFrames;
				player.breath = 200;
			}
			else if(respawnCounter > 0) respawnCounter--;
			else
			{
				resetExplosion(player.props.x + (player.props.width >> 1), player.props.y + (player.props.height >> 1));
				respawnCounter = 300;
			}
		}
		if(renderThisFrame)
		{
			render(true);
			dupdate();
			renderThisFrame = false;
		}
		else renderThisFrame = true;

		frames++;
		#ifndef DEBUGMODE
		world.timeTicks++;
		#endif
		if(world.timeTicks >= DAY_TICKS) world.timeTicks = 0;
		while(!*flag) sleep();
		*flag = 0;
	}
}

// frames increases 60 times per second, take into account
int main(void)
{
	int menuSelect;
	extern font_t font_smalltext;
	extern bopti_image_t img_splash;
	int w, h;
	int timer;
	volatile int flag = 0;
	int mediaFree[2];
	bool doSave;

	switch(gint[HWCALC])
	{
		case HWCALC_FX9860G_SH4:
			game = (struct GameCompatibilityPresets) {
				.HWMODE = MODE_RAM,
				.RAM_START = (void *)0x88040000,
				.WORLD_WIDTH = 1000,
				.WORLD_HEIGHT = 250,
				.WORLDGEN_MULTIPLIER = 1.0
			};
			break;
		
		case HWCALC_G35PE2:
			spu_zero();
			game = (struct GameCompatibilityPresets) {
				.HWMODE = MODE_PRAM,
				.RAM_START = (void *)0xFE200000,
				.WORLD_WIDTH = 640,
				.WORLD_HEIGHT = 250,
				.WORLDGEN_MULTIPLIER = 0.64
			};
			break;
		
		default:
			incompatibleMenu(gint[HWCALC]);
			return 0;
	}

	save = (struct SaveData){
		.tileDataSize = game.WORLD_HEIGHT * game.WORLD_WIDTH * sizeof(Tile),
		.regionsX = game.WORLD_WIDTH / REGION_SIZE + 1,
		.regionsY = game.WORLD_HEIGHT / REGION_SIZE + 1,
		.tileData = game.RAM_START,
		.error = -99,
	};

	if(!testRAM()) 
	{
		incompatibleMenu(-99);
		return 1;
	}

	srand(RTC_GetTicks());

	dgray(DGRAY_ON);

	dclear(C_WHITE);
	dimage(12, 4, &img_splash);
	dupdate();
	timer = timer_configure(TIMER_ANY, 3000 * 1000, GINT_CALL(NULL));
	timer_start(timer);
	timer_wait(timer);

	menuSelect = mainMenu();
	if(menuSelect == -1) return 1;
	
	player = (struct Player){

		.props = {
			.width = 12, 
			.height = 21
		},

		.combat = {
			.health = 100,
			.alignment = ALIGN_NEUTRAL,
			.immuneFrames = 40
		},

		.cursor = {75, 32},

		.inventory = {
			.getFirstFreeSlot = &getFirstFreeSlot,
			.removeItem = &removeItem,
			.stackItem = &stackItem,
			.tallyItem = &tallyItem,
			.findSlot = &findSlot,
			.getSelected = &getSelected
		},

		.tool = { TOOL_TYPE_NONE },
		.breath = 200
	};

	for(int slot = 0; slot < INVENTORY_SIZE; slot++) player.inventory.items[slot] = (Item){ITEM_NULL, PREFIX_NONE, 0};

	world = (struct World)
	{
		.tiles = (Tile*)save.tileData,

		.entities = malloc(MAX_ENTITIES * sizeof(Entity)),

		.explosion = {
			.numParticles = 50,
			.particles = malloc(50 * sizeof(Particle))
		},

		.chests = {
			.chests = malloc(MAX_CHESTS * sizeof(struct Chest)),
			.addChest = &addChest,
			.removeChest = &removeChest,
			.findChest = &findChest
		},

		.placeTile = &placeTile,
		.removeTile = &removeTile,
		
		.spawnEntity = &spawnEntity,
		.removeEntity = &removeEntity,
		.checkFreeEntitySlot = &checkFreeEntitySlot
	};
	allocCheck(world.entities);
	allocCheck(world.explosion.particles);
	allocCheck(world.chests.chests);

//	An entity ID of -1 is considered a free slot
	for(int idx = 0; idx < MAX_ENTITIES; idx++) world.entities[idx] = (Entity){ -1 };

	if(menuSelect == 0) // New game
	{
		generateWorld();
		memset(save.regionData, 1, save.regionsX * save.regionsY);
		player.inventory.items[0] = (Item){ITEM_COPPER_SWORD, rand() % PREFIX_COUNT, 1};
		player.inventory.items[1] = (Item){ITEM_COPPER_PICK, rand() % PREFIX_COUNT, 1};
		player.maxHealth = 100;
		world.timeTicks = timeToTicks(8, 15);
		setPlayerSpawn();
		addNPC(NPC_GUIDE);

		player.props.x = player.spawn.x;
		player.props.y = player.spawn.y;
	} 
	else if(menuSelect == 1) // Load game
	{
		dgray(DGRAY_OFF);
		dclear(C_WHITE);
		dupdate();

		gint_world_switch(GINT_CALL(&getVersionInfo));
		if(strcmp(versionBuffer, VERSION) != 0) saveVersionDifferenceMenu(versionBuffer);

		dclear(C_WHITE);
		dsize("Loading World...", NULL, &w, &h);
		dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Loading World...");
		dupdate();
		gint_world_switch(GINT_CALL(&loadSave));
		if(save.error != -99) 
		{
			loadFailMenu();
			return 1;
		}

		dgray(DGRAY_ON);
	}

	dclear(C_WHITE);
	dupdate();

	dfont(&font_smalltext);

	timer = timer_configure(TIMER_ANY, (1000 / 60) * 1000, GINT_CALL(&frameCallback, &flag));
	timer_start(timer);

	fillVarBuffer(0, 0, VAR_BUF_WIDTH, VAR_BUF_HEIGHT);
	
	registerEquipment();
	registerHeld();

	// Do the game
	doSave = gameLoop(&flag);

	timer_stop(timer);
	if(doSave)
	{
		dgray(DGRAY_OFF);
		dclear(C_WHITE);
		dfont(NULL);
		dsize("Saving World...", NULL, &w, &h);
		dtext(64 - w / 2, 32 - h / 2, C_BLACK, "Saving World...");
		dupdate();
	}

	//int nums[] = {world.entities, world.chests.chests, world.explosion.particles, world.npcs, world.markers};
	//debugNumberMenu(nums, 5);

	free(world.entities);
	free(world.explosion.particles);

	if(doSave) gint_world_switch(GINT_CALL(&saveGame));
	free(world.chests.chests);
//	Nothing is allocated if there are no NPCs
	if(world.npcs != NULL) free(world.npcs);
	if(world.markers != NULL) free(world.markers);
	if(save.error != -99) saveFailMenu();
	
	if(doSave)
	{
		Bfile_GetMediaFree_OS(u"\\\\fls0", mediaFree);
		if(mediaFree[1] < 350000) lowSpaceMenu(mediaFree[1]);

#ifndef DEBUGMODE
		gint_world_switch(GINT_CALL(&JumpOptimising));
#endif
	}

//	Return to the main menu by restarting the addin
	//gint_world_switch((void (*)())0x00300200);

	return 1;
}
