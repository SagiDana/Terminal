#ifndef FONT_H
#define FONT_H

#include "common.h"

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>


typedef struct{
    XftFont* xft_font;
    int width;
    int height;
}TFont;

TFont* font_create(  Display* display,
                     int screen,
                     char* font_name, 
                     double font_size);
void font_destroy(TFont* font);

#endif
