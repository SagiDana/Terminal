#ifndef FONT_H
#define FONT_H

// An amaizing explaination about fontconfig and the history:
// https://unix.stackexchange.com/questions/338047/how-does-fontconfig-actually-work

#include "common.h"

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>


typedef struct{
    XftFont* normal_font;
    XftFont* italic_font;
    XftFont* bold_font;
    XftFont* italic_bold_font;
    int width;
    int height;
}TFont;

TFont* font_create(  Display* display,
                     int screen,
                     char* font_name, 
                     double font_size);
void font_destroy(TFont* font);

XftFont* font_get( Display* display,
                   int screen,
                   char* font_name, 
                   double font_size,
                   unsigned int style);

#endif
