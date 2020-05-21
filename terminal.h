#ifndef TERMINAL_H
#define TERMINAL_H


#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "common.h"


typedef struct{
    Display* display;
    int screen;
    Visual* visual;
    Colormap colormap;
    Window window;

    XColor background_color;

    XftDraw* xft_draw;
    XftFont* xft_font;

    int x;
    int y;
    unsigned int width;
    unsigned int height;
}Terminal;

static Terminal terminal;


int main();


// -----------------------------------------------------------------------
// colors
// -----------------------------------------------------------------------
char background_color[] = "#000000";

#endif
