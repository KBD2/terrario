#!/usr/bin/sh

gcc -Llib -Iinclude world.c -O3 -o world -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -g
