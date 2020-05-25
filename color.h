#ifndef COLOR_H
#define COLOR_H

#include "common.h"

#define TRUE_COLOR_COLOR(r,g,b) ((1 << 24) | (r << 16) | (g << 8) | (b))

#define TRUE_COLOR_RED_16BIT(tc) (((tc) & 0xFF0000) >> 8)
#define TRUE_COLOR_GREEN_16BIT(tc) ((tc) & 0xFF00)
#define TRUE_COLOR_BLUE_16BIT(tc) (((tc) & 0xFF) << 8)

int color_from_string(char* string, unsigned int* true_color);

unsigned int map_4bit_to_true_color(unsigned int color);

#endif
