#include <stdlib.h>
#include <string.h>

#include <gint/gray.h>
#include <gint/defs/util.h>
#include <gint/keyboard.h>
#include <gint/std/stdlib.h>

#include "npc.h"
#include "world.h"
#include "render.h"
#include "generate.h"

char *guideDialogue[] = {
	"Greetings, player. Is there something I can help you with?",
	"I am here to give you advice on what to do next. It is recommended that you talk with me anytime you get stuck.",
	"They say there is a person who will tell you how to survive in this land... oh wait. That's me."
};

char *guideHelpDialogue[] = {
	"Underground exploration can yield valuable treasures!",
	"A house can be a useful refuge from the beasts that roam at night.",
	"Ropes can be used to traverse pits!",
	"You can use [TAN] to mark a valid house as suitable for NPCs!"
};

char *nurseDialogue[] = {
	"Turn your head and cough.",
	"That's not the biggest I've ever seen... Yes, I've seen bigger wounds for sure.",
	"Would you like a lollipop?",
	"Show me where it hurts."
};

char *nurseHealDialogue[] = {
	"That didn't hurt too bad, now did it?",
	"All better. I don't want to see you jumping off anymore cliffs.",
	"That's probably going to leave a scar.",
	"I managed to sew your face back on. Be more careful next time.",
	"I don't give happy endings.",
	"I can't do anymore for you without plastic surgery.",
	"Quit wasting my time."
};

void guideMenu()
{
	while(npcTalk(sizeof(guideHelpDialogue) / sizeof(char*), guideHelpDialogue, MENU_GUIDE));
}

void nurseMenu()
{
	bool cont;
	char **dialogue;
	int numDialogue;
	do
	{
		if(player.combat.health < player.maxHealth)
		{
			numDialogue = 1;
			if(player.combat.health >= 0.75 * player.maxHealth) dialogue = &nurseHealDialogue[0];
			else if(player.combat.health >= 0.5 * player.maxHealth) dialogue = &nurseHealDialogue[1];
			else if(player.combat.health >= 0.25 * player.maxHealth) dialogue = &nurseHealDialogue[2];
			else dialogue = &nurseHealDialogue[3];
			player.combat.health = player.maxHealth;
		}
		else
		{
			numDialogue = 3;
			dialogue = &nurseHealDialogue[4];
		}

		cont = npcTalk(numDialogue, dialogue, MENU_NURSE);
	}
	while(cont);
}

bool isNPCAlive(enum NPCs id)
{
	for(int i = 0; i < world.numNPCs; i++)
	{
		if(world.npcs[i].id == id) return true;
	}

	return false;
}

void addNPC(enum NPCs id)
{
	extern bopti_image_t img_npcs_guide, img_npcs_head_guide, img_npcs_nurse, img_npcs_head_nurse;
	NPC *npc;
	int tileX, tileY = 0;

	world.numNPCs++;
	world.npcs = realloc(world.npcs, world.numNPCs * sizeof(NPC));
	allocCheck(world.npcs);

	npc = &world.npcs[world.numNPCs - 1];

	switch(id)
	{
		case NPC_GUIDE:
			*npc = (NPC) {
				.sprite = &img_npcs_guide,
				.head = &img_npcs_head_guide,
				.props = {
					.width = 16,
					.height = 23
				},
				.numInteractDialogue = sizeof(guideDialogue) / sizeof(char*),
				.interactDialogue = guideDialogue,
				.menu = &guideMenu,
				.menuType = MENU_GUIDE
			};
			break;
		
		case NPC_NURSE:
			*npc = (NPC) {
				.sprite = &img_npcs_nurse,
				.head = &img_npcs_head_nurse,
				.props = {
					.width = 14,
					.height = 23
				},
				.numInteractDialogue = sizeof(nurseDialogue) / sizeof(char*),
				.interactDialogue = nurseDialogue,
				.menu = &nurseMenu,
				.menuType = MENU_NURSE
			};
			break;

//		Something's gone wrong!
		default:
			dclear(C_WHITE);
			dprint(0, 0, C_BLACK, "Invalid NPC %d", id);
			dupdate();
			while(1){getkey();}
			break;
	}

	npc->marker = -1;
	npc->id = id;

	tileX = game.WORLD_WIDTH >> 1;
	while(getTile(tileX, tileY).id == TILE_NOTHING) tileY++;
	tileY -= 4;
	npc->props.x = tileX << 3;
	npc->props.y = tileY << 3;
}

void doNPCHouseCheck()
{
	NPC *npc;
	HouseMarker *marker;
	bool spawnNPC = false;
	enum NPCs npcToAdd;

	if(!isNPCAlive(NPC_GUIDE))
	{
		spawnNPC = true;
		npcToAdd = NPC_GUIDE;
	}
	else if(!isNPCAlive(NPC_NURSE) && player.combat.health > 100)
	{
		spawnNPC = true;
		npcToAdd = NPC_NURSE;
	}

	if(!spawnNPC) return;

	addNPC(npcToAdd);
	npc = &world.npcs[world.numNPCs - 1];
	for(int i = 0; i < world.numMarkers; i++)
	{
		marker = &world.markers[i];
		if(marker->occupant == -1)
		{
			marker->occupant = world.numNPCs - 1;
			npc->marker = i;
			npc->props.x = marker->position.x << 3;
			npc->props.y = marker->position.y << 3;
			break;
		}
	}
}

void npcUpdate(int frames)
{
	NPC *npc;
	HouseMarker *marker;
	bool distancedFromPlayer;
	bool distancedFromMarker;
	int walkMaxFrame;

	if(isDay() && frames % 1200 == 1199) doNPCHouseCheck();

	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		npc = &world.npcs[idx];

		switch(npc->id)
		{
			case NPC_GUIDE:
				walkMaxFrame = 15;
				break;
			
			case NPC_NURSE:
				walkMaxFrame = 12;
				break;
			
			default:
				walkMaxFrame = 0;
				break;
		}

//		Housing
		if(frames % 300 == 0)
		{
			if(npc->marker == -1)
			{
				if(isDay())
				{
					for(int i = 0; i < world.numMarkers; i++)
					{
						marker = &world.markers[i];
						if(marker->occupant == -1)
						{
							marker->occupant = idx;
							npc->marker = i;
							break;
						}
					}
				}
			}
			else if(!isDay())
			{
				distancedFromPlayer = abs(npc->props.x - player.props.x) > SCREEN_WIDTH || abs(npc->props.y - player.props.y) > SCREEN_HEIGHT;
				distancedFromMarker = abs((world.markers[npc->marker].position.x << 3) - player.props.x) > (SCREEN_WIDTH >> 1) + 16 || abs((world.markers[npc->marker].position.y << 3) - player.props.y) > (SCREEN_HEIGHT >> 1) + 20;
				if(distancedFromPlayer && distancedFromMarker)
				{
					npc->props.x = (world.markers[npc->marker].position.x << 3);
					npc->props.y = (world.markers[npc->marker].position.y << 3);
				}
			}
		}

//		Physics and movement

		handlePhysics(&npc->props, frames, false, WATER_FRICTION);

		if(npc->props.movingSelf)
		{
			if(rand() % 50 == 0) npc->props.movingSelf = false;
		}
		else if(rand() % 300 == 0)
		{
			npc->props.movingSelf = true;
			npc->props.xVel = (rand() % 2) ? -0.3 : 0.3;
			npc->anim.direction = npc->props.xVel < 0;
		}

//		Animation

		if(npc->props.yVel != 0)
		{
			npc->anim.animation = 1;
			npc->anim.animationFrame = 1;
		}
		else if(npc->props.xVel != 0)
		{
			if(npc->anim.animation != 2)
			{
				npc->anim.animation = 2;
				npc->anim.animationFrame = 2;
			}
		}
		else
		{
			npc->anim.animation = 0;
			npc->anim.animationFrame = 0;
		}

//		Walking animation is the only one with multiple frames
		if(frames & 1 && npc->anim.animation == 2)
		{
			npc->anim.animationFrame++;
			if(npc->anim.animationFrame > walkMaxFrame) npc->anim.animationFrame = 2;
		}
	}
}

bool npcTalk(int numDialogue, char **dialogue, enum MenuTypes type)
{
	char buffer[33];
	extern bopti_image_t img_ui_npctalk;
	key_event_t key;
	int selectedLine = rand() % numDialogue;
	int lines = strlen(dialogue[selectedLine]) / 32 + 1;

	buffer[32] = '\0';

	render(false);

	drect_border(0, 0, 127, 7 * lines + 1, C_WHITE, 1, C_BLACK);

	for(int line = 0; line < lines; line++)
	{
		strncpy(buffer, (char *)(dialogue[selectedLine] + 32 * line), 32);
		dprint(2, line * 7 + 2, C_BLACK, buffer);
	}

	switch(type)
	{
		case MENU_GUIDE:
			dsubimage(0, 7 * lines + 2, &img_ui_npctalk, 17, 0, 17, 7, DIMAGE_NONE);
			break;
		
		case MENU_SHOP:
			dsubimage(0, 7 * lines + 2, &img_ui_npctalk, 0, 0, 17, 7, DIMAGE_NONE);
			break;
		
		case MENU_NURSE:
			dsubimage(0, 7 * lines + 2, &img_ui_npctalk, 52, 0, 17, 7, DIMAGE_NONE);
			break;
	}
	dsubimage(18, 7 * lines + 2, &img_ui_npctalk, 34, 0, 17, 7, DIMAGE_NONE);

	dupdate();

	while(true)
	{
		key = getkey_opt(GETKEY_NONE, NULL);
		
		switch(key.key)
		{
			case KEY_F1:
				return true;

			case KEY_F2:
				return false;

			default:
				break;
		}
	}
}

bool checkHousingValid(Coords position)
{
	short idx = 0;
	int deltas[4][2] = {
		{-1, 0},
		{1, 0},
		{0, -1},
		{0, 1}
	};
	bool checkAdjacent;
	short end = 0;
	Coords toCheck;
	bool isAlreadyPresent;
	short x, y;

	bool hasChair = false;
	bool hasTable = false;
	bool hasLight = false;

	for(int i = 0; i < CHECK_BUFFER_SIZE; i++) checkCoords[i] = (Coords){-1, -1};

	checkCoords[idx] = position;

	while(checkCoords[idx].x != -1)
	{
		x = checkCoords[idx].x;
		y = checkCoords[idx].y;

		if(x < 0 || x >= game.WORLD_WIDTH || y < 0 || y >= game.WORLD_HEIGHT) continue;

		for(int i = 0; i < world.numMarkers; i++)
		{
			if(world.markers[i].position.x == x && world.markers[i].position.y == y)
			{
				return false;
			}
		}
		checkAdjacent = true;
		switch(getTile(x, y).id)
		{
			case TILE_NOTHING:
				break;
			
			case TILE_CHAIR_L:
			case TILE_CHAIR_R:
				hasChair = true;
				break;
			
			case TILE_WBENCH_L:
			case TILE_WBENCH_R:
				hasTable = true;
				break;
			
			case TILE_TORCH:
				hasLight = true;
				break;
			
			default:
				checkAdjacent = false;
				break;
		}

		if(checkAdjacent)
		{
			for(int i = 0; i < 4; i++)
			{
				toCheck = (Coords){x + deltas[i][0], y + deltas[i][1]};
				isAlreadyPresent = false;
				for(int check = 0; check <= end; check++)
				{
					if(toCheck.x == checkCoords[check].x && toCheck.y == checkCoords[check].y)
					{
						isAlreadyPresent = true;
						break;
					}
				}
				if(isAlreadyPresent) continue;
				end++;
				if(end == CHECK_BUFFER_SIZE) return false;
				checkCoords[end] = toCheck;
			}
		}

		idx++;
	}

	return idx >= 60 && hasChair && hasTable && hasLight;
}

void addMarker(Coords position)
{
	world.numMarkers++;
	world.markers = realloc(world.markers, world.numMarkers * sizeof(HouseMarker));
	allocCheck(world.markers);
	world.markers[world.numMarkers - 1] = (HouseMarker) {
		.position = position,
		.occupant = -1
	};
}

bool removeMarker(int idx)
{
	HouseMarker *marker = &world.markers[idx];

	if(idx < 0 || idx >= world.numMarkers) return false;

	if(marker->occupant != -1) world.npcs[marker->occupant].marker = -1;
	*marker = world.markers[world.numMarkers - 1];
	if(marker->occupant != -1) world.npcs[marker->occupant].marker = idx;
	world.numMarkers--;
	world.markers = realloc(world.markers, world.numMarkers * sizeof(HouseMarker));
	if(world.numMarkers > 0) allocCheck(world.markers);

	return true;
}

void updateMarkerChecks(Coords position)
{
	HouseMarker *marker;

	for(int idx = 0; idx < world.numMarkers; idx++)
	{
		marker = &world.markers[idx];
		if(abs(marker->position.x - position.x) < MARKER_CHECK_DISTANCE && abs(marker->position.y - position.y) < MARKER_CHECK_DISTANCE)
		{
			marker->doCheck = true;
		}
	}
}

void doMarkerChecks()
{
	HouseMarker *marker;
	Coords save;

	for(int idx = 0; idx < world.numMarkers; idx++)
	{
		marker = &world.markers[idx];
		if(marker->doCheck)
		{
			save = marker->position;
			marker->position = (Coords){-1, -1};
			if(!checkHousingValid(save))
			{
				removeMarker(idx);
//				Last marker will have been moved to current index
				idx--;
			}
			else
			{
				marker->doCheck = false;
				marker->position = save;
			}
		}
	}
}