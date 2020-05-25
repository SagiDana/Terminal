#include "color.h"

#include <string.h>
#include <stdlib.h>

/*
 * Visual Studio Code Mappings
 */
unsigned int colors_map[] = {
    [0] = TRUE_COLOR_COLOR(0, 0, 0),
    [1] = TRUE_COLOR_COLOR(205, 49, 49),
    [2] = TRUE_COLOR_COLOR(13, 188, 121),
    [3] = TRUE_COLOR_COLOR(229, 229, 16),
    [4] = TRUE_COLOR_COLOR(36, 114, 200),
    [5] = TRUE_COLOR_COLOR(188, 63, 188),
    [6] = TRUE_COLOR_COLOR(17, 168, 205),
    [7] = TRUE_COLOR_COLOR(229, 229, 229),
    [8] = TRUE_COLOR_COLOR(102, 102, 102),
    [9] = TRUE_COLOR_COLOR(241, 76, 76),
    [10] = TRUE_COLOR_COLOR(35, 209, 139),
    [11] = TRUE_COLOR_COLOR(245, 245, 67),
    [12] = TRUE_COLOR_COLOR(59, 142, 234),
    [13] = TRUE_COLOR_COLOR(214, 112, 214),
    [14] = TRUE_COLOR_COLOR(41, 184, 219),
    [15] = TRUE_COLOR_COLOR(229, 229, 229)
};

int color_from_string(char* string, unsigned int* true_color){
    int red, green, blue;
    char holder[3] = { 0 };

    ASSERT((strlen(string) == 7), "color string not in right length.\n");
    ASSERT((string[0] == '#'), "color string not in right length.\n");

    holder[0] = string[1];
    holder[1] = string[2];
    red = strtol(holder, NULL, 16);

    holder[0] = string[3];
    holder[1] = string[4];
    green = strtol(holder, NULL, 16);

    holder[0] = string[5];
    holder[1] = string[6];
    blue = strtol(holder, NULL, 16);

    *true_color = TRUE_COLOR_COLOR(red, green, blue);

    return 0;
fail:
    return -1;
}

unsigned int map_4bit_to_true_color(int color){
    if (BETWEEN(color, 30, 37)){
        return colors_map[color - 30];
    }
    if (BETWEEN(color, 90, 97)){
        return colors_map[color - 90];
    }
    if (BETWEEN(color, 40, 47)){
        return colors_map[color - 40];
    }
    if (BETWEEN(color, 100, 107)){
        return colors_map[color - 100];
    }
    return 0;
}
