#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/gray.h>

int main(void)
{
	extern image_t img_tile_stone;

	gray_start();
	gclear(C_WHITE);
	for(int y = 0; y < 64; y += 8)
	{
		for(int x = 0; x < 128; x += 8)
		{
			gimage(x, y, &img_tile_stone);
		}
	}
	gupdate();

	getkey();
	gray_stop();

	return 1;
}
