#include <gint/keyboard.h>
#include <gint/defs/util.h>

#include "entity.h"
#include "map.h"

// Syscalls
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070
typedef void(*sc_vv)(void);
const unsigned int sc0236[] = { SCA, SCB, SCE, 0x0236 };
#define RebootOS (*(sc_vv)sc0236)

void updatePlayer(struct Player* self)
{
	clearevents();
	if(keydown(KEY_LEFT)) self->x -= 4;
	if(keydown(KEY_RIGHT)) self->x += 4;
	if(keydown(KEY_UP)) self->y -= 4;
	if(keydown(KEY_DOWN)) self->y += 4;
//	Easiest way to exit from here
	if(keydown(KEY_MENU)) RebootOS();
	self->x = min(max(self->x, 0), 8 * MAP_WIDTH - self->width);
	self->y = min(max(self->y, 0), 8 * MAP_HEIGHT - self->height);
}