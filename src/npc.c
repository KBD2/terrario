#include <gint/std/stdlib.h>
#include <gint/gray.h>
#include <gint/keyboard.h>
#include <gint/std/string.h>
#include <gint/defs/util.h>

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

void guideMenu()
{
	while(npcTalk(sizeof(guideHelpDialogue) / sizeof(char*), guideHelpDialogue, MENU_GUIDE));
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
	extern bopti_image_t img_npcs_guide, img_npcs_head_guide;
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
	}

	npc->marker = NULL;

	tileX = game.WORLD_WIDTH >> 1;
	while(getTile(tileX, tileY).id == TILE_NOTHING) tileY++;
	tileY -= 4;
	npc->props.x = tileX << 3;
	npc->props.y = tileY << 3;
}

void npcUpdate(int frames)
{
	NPC *npc;
	HouseMarker *marker;
	bool distancedFromPlayer;
	bool distancedFromMarker;

	for(int idx = 0; idx < world.numNPCs; idx++)
	{
		npc = &world.npcs[idx];

//		Housing
		if(frames % 300 == 0)
		{
			if(npc->marker == NULL)
			{
				if(world.timeTicks > timeToTicks(4, 30) && world.timeTicks < timeToTicks(19, 30))
				{
					for(int i = 0; i < world.numMarkers; i++)
					{
						marker = &world.markers[i];
						if(marker->occupant == NULL)
						{
							marker->occupant = npc;
							npc->marker = marker;
							break;
						}
					}
				}
			}
			else if(world.timeTicks > timeToTicks(19, 30) || world.timeTicks < timeToTicks(4, 30))
			{
				distancedFromPlayer = abs(npc->props.x - player.props.x) > SCREEN_WIDTH || abs(npc->props.y - player.props.y) > SCREEN_HEIGHT;
				distancedFromMarker = abs((npc->marker->position.x << 3) - player.props.x) > (SCREEN_WIDTH >> 1) + 16 || abs((npc->marker->position.y << 3) - player.props.y) > (SCREEN_HEIGHT >> 1) + 20;
				if(distancedFromPlayer && distancedFromMarker)
				{
					npc->props.x = (npc->marker->position.x << 3);
					npc->props.y = (npc->marker->position.y << 3);
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
			if(npc->anim.animationFrame > 15) npc->anim.animationFrame = 2;
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
		
		case MENU_HEAL:
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
	world.markers = realloc(world.markers, sizeof(world.numMarkers) * sizeof(HouseMarker));
	allocCheck(world.markers);
	world.markers[world.numMarkers - 1] = (HouseMarker) {
		.position = position
	};
}

bool removeMarker(Coords position)
{
	HouseMarker *marker;

	for(int idx = 0; idx < world.numMarkers; idx++)
	{
		marker = &world.markers[idx];
		if(marker->position.x == position.x && marker->position.y == position.y)
		{
			*marker = world.markers[world.numMarkers - 1];
			world.numMarkers--;
			world.markers = realloc(world.markers, sizeof(world.numMarkers) * sizeof(HouseMarker));
			allocCheck(world.markers);
			return true;
		}
	}
	return false;
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

	for(int idx = world.numMarkers - 1; idx >= 0; idx--)
	{
		marker = &world.markers[idx];
		if(marker->doCheck)
		{
			if(!checkHousingValid(marker->position))
			{
				if(marker->occupant != NULL) marker->occupant->marker = NULL;
				removeMarker(marker->position);
				continue;
			}
			marker->doCheck = false;
		}
	}
}