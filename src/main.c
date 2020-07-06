#include <gint/gray.h>

#include "map.h"
#include "entity.h"
#include "render.h"

// Syscalls
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070
typedef int(*sc_iv)(void);
typedef int (*sc_iii)(int, int);
const unsigned int sc003B[] = { SCA, SCB, SCE, 0x003B };
const unsigned int sc003C[] = { SCA, SCB, SCE, 0x003C };
#define RTC_GetTicks (*(sc_iv)sc003B)
#define RTC_Elapsed_ms (*(sc_iii)sc003C)

int main(void)
{
	struct Map map;
	struct Player player = {0, 20 << 3, 12, 21, &updatePlayer};
	int ticks;

	generateMap(&map);

	gray_delays(920, 1740);
	gray_start();

	while(1)
	{
		ticks = RTC_GetTicks();
		player.update(&player);
		render(&map, &player);
//		Arbitrary 30FPS, just felt right
		while(!RTC_Elapsed_ms(ticks, 33)){}
	}

	return 1;
}
