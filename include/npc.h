#pragma once

#include <gint/gray.h>

#include "entity.h"

enum NPCs {
	NPC_GUIDE
};

enum MenuTypes {
	MENU_SHOP,
	MENU_GUIDE,
	MENU_HEAL
};

typedef struct {
	enum NPCs id;
	bopti_image_t *sprite;
	bopti_image_t *head;
	char *name;
	struct EntityPhysicsProps props;
	struct AnimationData anim;
	int numInteractDialogue;
	char **interactDialogue;
	void (*menu)();
	enum MenuTypes menuType;
	Coords house;
} NPC;

struct HouseMarker {
	Coords position;
	NPC *occupant;
};

bool isNPCAlive(enum NPCs id);

void addNPC(enum NPCs id);

bool removeNPC(enum NPCs id);

void npcUpdate(int frames);

bool npcTalk(int numDialogue, char **dialogue, enum MenuTypes type);

bool checkHousingValid(short x, short y);

void addMarker(Coords position, NPC *npc);

bool removeMarker(Coords position);