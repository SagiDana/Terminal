#ifndef T_H
#define T_H


#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <limits.h>

#include "common.h"
#include "terminal.h"
#include "element.h"
#include "pty.h"
#include "font.h"


typedef struct{
    Display* display;
    int screen;
    Visual* visual;
    Colormap colormap;
    Window window;
    Drawable drawable;

    XftColor background_color;
    XftColor foreground_color;

    XftDraw* xft_draw;
    TFont* font;

    Terminal* terminal;

    TPty* pty;

    int x;
    int y;
    unsigned int width;
    unsigned int height;
}XTerminal;

static XTerminal xterminal;


int main();

// -----------------------------------------------------------------------
// configuration
// -----------------------------------------------------------------------
char shell[] = "/bin/sh";

char font_name[] = "mononoki";
double font_size = 12.0;

unsigned int cols = 80;
unsigned int rows = 24;

// -----------------------------------------------------------------------
// colors
// -----------------------------------------------------------------------
char background_color[] = "#000011";
char foreground_color[] = "#FFCC00";

// -----------------------------------------------------------------------
// keys
// -----------------------------------------------------------------------
typedef struct{
    KeySym keysym;
    unsigned int mod;
    char* string;
} Key;

#define XK_ANY_MOD (UINT_MAX)

static Key terminal_keys[] = {
	{ XK_KP_Enter,      XK_ANY_MOD,     "\r" }
};

#endif
