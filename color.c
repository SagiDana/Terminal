#include "color.h"

#include <string.h>
#include <stdlib.h>


// ---------------------------------------------------------
// Visual Studio Code Mappings
// ---------------------------------------------------------
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
// ---------------------------------------------------------

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

unsigned int map_4bit_to_true_color(unsigned int color){
    if (!BETWEEN(color, 0, 15)){
        return 0;
    }
    return colors_map[color];
}


// ---------------------------------------------------------
// xterm color mapping.
// ---------------------------------------------------------

unsigned int xterm_256_color_mapping[] = {
    0x0, 0x387dff5f, 0x39769f87, 0x3acfefaf, 0x3bfdffd7, 
    0x3cdaffff, 0x3dff5f00, 0x3f675f5f, 0x406fc787, 0x4173ffaf, 
    0x427ef7d7, 0x43fcffff, 0x449f8700, 0x45bf7f5f, 0x46d4ef87, 
    0x47fbbfaf, 0x49afdfd7, 0x4aadffff, 0x4befaf00, 0x4cfe7f5f, 
    0x4dbf9787, 0x4ee3efaf, 0x4fcbf7d7, 0x50dfffff, 0x51ffd700, 
    0x53dfdf5f, 0x54f7bf87, 0x55ffefaf, 0x56f2dfd7, 0x57f9ffff, 
    0x58ffff00, 0x59cfff5f, 0x5bede787, 0x5d16bfaf, 0x5d3bd7d7, 
    0x5f61ffff, 0x5f5f0000, 0x607dff5f, 0x61de9f87, 0x62ffefaf, 
    0x63d5ffd7, 0x64faffff, 0x665f5f00, 0x676f5f5f, 0x686fc787, 
    0x697bffaf, 0x6afef7d7, 0x6ba4ffff, 0x6cc78700, 0x6ddf7f5f, 
    0x6efcef87, 0x709bbfaf, 0x71b7dfd7, 0x72fdffff, 0x73ffaf00, 
    0x74b67f5f, 0x75bf9787, 0x76cbefaf, 0x77ebf7d7, 0x79d7ffff, 
    0x7af7d700, 0x7bdfdf5f, 0x7cffbf87, 0x7dffefaf, 0x7efadfd7, 
    0x7ff9ffff, 0x80ffff00, 0x81f7ff5f, 0x831de787, 0x853ebfaf, 
    0x855bd7d7, 0x8769ffff, 0x87870000, 0x88bdff5f, 0x89c69f87, 
    0x8adfefaf, 0x8bfdffd7, 0x8d5affff, 0x8e7f5f00, 0x8f775f5f, 
    0x907fc787, 0x91f3ffaf, 0x92fef7d7, 0x93ccffff, 0x94ef8700, 
    0x968f7f5f, 0x97b4ef87, 0x98dbbfaf, 0x99ffdfd7, 0x9afdffff, 
    0x9bbfaf00, 0x9cbe7f5f, 0x9dff9787, 0x9ef3efaf, 0xa0dbf7d7, 
    0xa1ffffff, 0xa2dfd700, 0xa3ffdf5f, 0xa4e7bf87, 0xa5efefaf, 
    0xa6f2dfd7, 0xa7f9ffff, 0xa8ffff00, 0xab1fff5f, 0xab3de787, 
    0xad56bfaf, 0xad7bd7d7, 0xafb1ffff, 0xafaf0000, 0xb0fdff5f, 
    0xb1ee9f87, 0xb34fefaf, 0xb475ffd7, 0xb55affff, 0xb67f5f00, 
    0xb77f5f5f, 0xb8ffc787, 0xb9fbffaf, 0xbafef7d7, 0xbbf4ffff, 
    0xbd978700, 0xbeaf7f5f, 0xbfdcef87, 0xc0fbbfaf, 0xc1a7dfd7, 
    0xc2adffff, 0xc3efaf00, 0xc4f67f5f, 0xc5ff9787, 0xc7dbefaf, 
    0xc8fbf7d7, 0xc9d7ffff, 0xcaf7d700, 0xcbdfdf5f, 0xccefbf87, 
    0xcdefefaf, 0xcefadfd7, 0xd0f9ffff, 0xd1ffff00, 0xd347ff5f, 
    0xd36de787, 0xd57ebfaf, 0xd59bd7d7, 0xd7b9ffff, 0xd7d70000, 
    0xd8fdff5f, 0xda569f87, 0xdb6fefaf, 0xdc5dffd7, 0xdd7affff, 
    0xdedf5f00, 0xdfe75f5f, 0xe0efc787, 0xe1f3ffaf, 0xe2fef7d7, 
    0xe49cffff, 0xe5bf8700, 0xe6df7f5f, 0xe7f4ef87, 0xe89bbfaf, 
    0xe9afdfd7, 0xeaedffff, 0xebefaf00, 0xedbe7f5f, 0xeebf9787, 
    0xefc3efaf, 0xf0ebf7d7, 0xf1ffffff, 0xf2dfd700, 0xf3ffdf5f, 
    0xf4f7bf87, 0xf5ffefaf, 0xf7f2dfd7, 0xf8f9ffff, 0xf9ffff00, 
    0xfb6fff5f, 0xfb8de787, 0xfdb6bfaf, 0xfddbd7d7, 0xffe1ffff, 
    0xffff0000, 0x13dff5f, 0x27e9f87, 0x35fefaf, 0x475ffd7, 
    0x5daffff, 0x6ff5f00, 0x7ef5f5f, 0x8efc787, 0xa7bffaf, 
    0xb7ef7d7, 0xcc4ffff, 0xde78700, 0xeff7f5f, 0xf9cef87, 
    0x10bbbfaf, 0x11f7dfd7, 0x12fdffff, 0x14bfaf00, 0x15b67f5f, 
    0x16ff9787, 0x17ebefaf, 0x18cbf7d7, 0x19f7ffff, 0x1ad7d700, 
    0x1bffdf5f, 0xffffa7a4, 0xffffafae, 0xffffbfb8, 0xffffc3c2, 
    0xffffcfcc, 0xffffd7d6, 0xffffffe0, 0xffffebea, 0xfffff7f4, 
    0xfffffffe, 0x8080808, 0x12121212, 0x1c1c1c1c, 0x26262626, 
    0x30303030, 0x3a3a3a3a, 0x44444444, 0x4e4e4e4e, 0x58585858, 
    0x62626262, 0x6c6c6c6c, 0x76767676, 0x80808080, 0x8a8a8a8a, 
    0x94949494, 0x9e9e9e9e, 0xa8a8a8a8, 0xb2b2b2b2, 0xbcbcbcbc, 
    0xc6c6c6c6, 0xd0d0d0d0, 0xdadadada, 0xe4e4e4e4, 0xeeeeeeee
};

// ---------------------------------------------------------

unsigned int get_xterm_color(int color_index){
    if (color_index < 16){
        return 0;
    }
    return xterm_256_color_mapping[color_index - 16];
}
