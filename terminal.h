#ifndef TERMINAL_H
#define TERMINAL_H


#include "common.h"

typedef struct{
    Display* display;
    int screen;
    Visual* visual;
    Colormap colormap;
    Window window;

    int x;
    int y;
    unsigned int width;
    unsigned int height;
}Terminal;

static Terminal terminal;

int main();

#endif
