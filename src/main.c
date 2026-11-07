#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <gint/clock.h>
#include <gint/bfile.h>
#include <gint/hardware.h>
#include <gint/gray.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <fxlibc/printf.h>

#include "defs.h"
#include "syscalls.h"
#include "world.h"
#include "entity.h"
#include "render.h"
#include "menu.h"
#include "save.h"
#include "crafting.h"
#include "update.h"
#include "worldgen.h"
#include "stream.h"
#include "optimise.h"
#include "npc.h"
#include "chat.h"

// Syscalls
const unsigned int sc003B[] = { SCA, SCB, SCE, 0x003B };
const unsigned int sc019F[] = { SCA, SCB, SCE, 0x019F };
const unsigned int sc0236[] = { SCA, SCB, SCE, 0x0236 };
const unsigned int sc042E[] = { SCA, SCB, SCE, 0x042E };
const unsigned int sc015F[] = { SCA, SCB, SCE, 0x015F };

// Global variables
char varBuffer[VAR_BUF_HEIGHT][VAR_BUF_WIDTH];
struct SaveData save;
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

/* Puts the spawn point on the surface of the starting column */
static void setPlayerSpawn(int spawnX)
{
	int surface = wgSurfaceY(spawnX);

	player.spawn.x = spawnX * 8;
	player.spawn.y = surface * 8 - player.props.height - 8;
}

/* Drops the player onto solid ground after appearing in the window */
static void settlePlayer()
{
	int x = (player.props.x + (player.props.width >> 1)) >> 3;
	int y = player.props.y >> 3;

//	Climb out of the ground
	while(y > 0 && tiles[getTile(x, y + 2).id].physics == PHYS_SOLID) y--;
	player.props.y = y * 8;
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
		if(updateRet == UPDATE_EXIT) return true;
		else if(updateRet == UPDATE_EXIT_NOSAVE) return false;
		if(!respawnCounter) playerUpdate(frames);
		if(!streamUpdate())
		{
			save.error = stream.error;
			saveFailMenu();
			save.error = -99;
		}

		if(frames & 1 && world.explosion.numParticles > 0) updateExplosion();
		if(frames % 8 == 0) worldUpdate();
		if(frames % 3600 == 0) doMarkerChecks();

		doEntityCycle(frames);
		doSpawningCycle();
		npcUpdate(frames);
		if(player.combat.health <= 0)
		{
			if(respawnCounter == 1)
			{
				if(!streamTeleport(player.spawn.x, player.spawn.y))
				{
					save.error = -6;
					saveFailMenu();
					save.error = -99;
				}
				player.combat.health = player.maxHealth;
				player.props.xVel = 0;
				player.props.yVel = 0;
				player.props.dropping = false;
				respawnCounter = 0;
				player.combat.currImmuneFrames = player.combat.immuneFrames;
				player.breath = 200;
				player.pixelsFallen = 0;
			}
			else if(respawnCounter > 0) respawnCounter--;
			else
			{
				createExplosion(player.props.x + (player.props.width >> 1), player.props.y + (player.props.height >> 1));
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

		updateChat();

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
	int mediaFree[2] = {0, 0};
	bool doSave;

	__printf_enable_fixed();

	/* Every model keeps the same small chunk window in ordinary RAM; the world
	   itself is infinite (see stream.h). */
	switch(gint[HWCALC])
	{
		case HWCALC_FX9860G_SH4:
		case HWCALC_G35PE2:
			game = (struct GameCompatibilityPresets) {
				.WORLD_WIDTH = WINDOW_WIDTH,
				.WORLD_HEIGHT = WINDOW_HEIGHT
			};
			break;
		
		default:
			incompatibleMenu(gint[HWCALC]);
			return 0;
	}

	save = (struct SaveData){
		.tileDataSize = game.WORLD_HEIGHT * game.WORLD_WIDTH * sizeof(Tile),
		.error = -99,
	};

	/* 128x96 tiles = 12 kB of normal RAM */
	save.tileData = malloc(save.tileDataSize);
	allocCheck(save.tileData);
	memset(save.tileData, 0, save.tileDataSize);

	unsigned int seed = RTC_GetTicks();
	srand(seed);
//	Mix in another value so two new worlds started on the same RTC tick don't
//	reuse each other's chunk files.
	seed ^= (unsigned int)rand() << 16;
	srand(seed);

	dgray(DGRAY_ON);

	dclear(C_WHITE);
	dimage(12, 4, &img_splash);
	dupdate();
	timer = timer_configure(TIMER_ANY, 3000 * 1000, GINT_CALL(NULL));
	timer_start(timer);
	timer_wait(timer);

	menuSelect = mainMenu();
	if(menuSelect == -1) { dgray(DGRAY_OFF); return 1; }
	
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
			.particles = malloc(EXPLOSION_NUM_PARTICLES * sizeof(Particle))
		},

		.chests = {
			.addChest = &addChest,
			.removeChest = &removeChest,
			.findChest = &findChest,
			.flushChest = &flushChest,
			.closeChest = &closeChest
		},

		.placeTile = &placeTile,
		.removeTile = &removeTile,
		
		.spawnEntity = &spawnEntity,
		.removeEntity = &removeEntity,
		.checkFreeEntitySlot = &checkFreeEntitySlot
	};
	allocCheck(world.entities);
	allocCheck(world.explosion.particles);

//	An entity ID of -1 is considered a free slot
	for(int idx = 0; idx < MAX_ENTITIES; idx++) world.entities[idx] = (Entity){ -1 };

	if(menuSelect == 0) // New game
	{
		int spawnX;

		save.worldSeed = seed;
		wgSetSeed(seed);

		dclear(C_WHITE);
		dtext_opt(64, 32, C_BLACK, C_WHITE, DTEXT_CENTER, DTEXT_MIDDLE, "Clearing old world...");
		dupdate();
//		Chunk and chest files of a previous world would otherwise pile up
		gint_world_switch(GINT_CALL(&clearSaveFolder));

		spawnX = wgFindSpawnX();
		setPlayerSpawn(spawnX);
		if(!streamInit(player.spawn.x >> 3, player.spawn.y >> 3)) memoryErrorMenu();
		player.props.x = player.spawn.x - stream.originX * 8;
		player.props.y = player.spawn.y - stream.originY * 8;
		settlePlayer();
		player.spawn.y = player.props.y + stream.originY * 8;

		player.inventory.items[0] = (Item){ITEM_COPPER_SWORD, rand() % PREFIX_COUNT, 1};
		player.inventory.items[1] = (Item){ITEM_COPPER_PICK, rand() % PREFIX_COUNT, 1};
		player.maxHealth = 100;
		world.timeTicks = timeToTicks(8, 15);
		addNPC(NPC_GUIDE);
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
		finishLoad();
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

	free(world.entities);
	free(world.explosion.particles);

	if(lastMessage != NULL) clearMessage(lastMessage);

	if(doSave)
	{
		bool chestSaved = world.chests.flushChest();
		gint_world_switch(GINT_CALL(&saveGame));
		if(!chestSaved) save.error = -7;
	}
//	Nothing is allocated if there are no NPCs
	if(world.npcs != NULL) free(world.npcs);
	if(world.markers != NULL) free(world.markers);
	free(save.tileData);
	streamFree();
	if(save.error != -99) saveFailMenu();
	
	if(doSave)
	{
		Bfile_GetMediaFree_OS(u"\\\\fls0", mediaFree);
		if(mediaFree[1] < 350000) lowSpaceMenu(mediaFree[1]);

#ifndef DEBUGMODE
		gint_world_switch(GINT_CALL(&JumpOptimising));
#endif
	}

	return 1;
}
