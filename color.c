#include "color.h"

#include <string.h>
#include <stdlib.h>


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
