#pragma once

#define VERSION "v0.4.1-indev"

#define RAM_START 0x88040000
#define REGION_SIZE 96

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define WORLD_WIDTH 1000
#define WORLD_HEIGHT 250
#define GRAVITY_ACCEL 0.15

#define INVENTORY_SIZE 24

int frameCallback(int* flag);