#pragma once

#include <gint/gray.h>

#include "entity.h"

enum NPCs {
	NPC_NONE,
	NPC_GUIDE,
	NPC_NURSE
};

enum MenuTypes {
	MENU_SHOP,
	MENU_GUIDE,
	MENU_NURSE
};

struct HouseMarker;

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
	short marker;
} NPC;

typedef struct HouseMarker {
	Coords position;
	short occupant;
	bool doCheck;
} HouseMarker;

bool isNPCAlive(enum NPCs id);

void addNPC(enum NPCs id);

bool removeNPC(enum NPCs id);

void npcUpdate(int frames);

bool npcTalk(int numDialogue, char **dialogue, enum MenuTypes type);

bool checkHousingValid(Coords position);

void addMarker(Coords position);

bool removeMarker(int idx);

void updateMarkerChecks(Coords position);

void doMarkerChecks();

void doNPCHouseCheck();