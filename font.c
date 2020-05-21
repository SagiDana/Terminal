#include "font.h"

#include <stdlib.h>


TFont* font_create(  Display* display,
                     int screen,
                     char* font_name, 
                     double font_size){

    TFont* font = NULL;

    font = (TFont*) malloc(sizeof(TFont));
    ASSERT(font, "failed to malloc font.\n");

    font->xft_font = XftFontOpen(   display,
                                    screen,
                                    XFT_FAMILY, XftTypeString, font_name,
                                    XFT_SIZE, XftTypeDouble, font_size,
                                    NULL);

    // all printable characters
    char ascii_printable[] =    " !\"#$%&'()*+,-./0123456789:;<=>?"
                                "@abcdefghijklmnopqrstuvwxyz[\\]^_"
                                "`abcdefghijklmnopqrstuvwxyz{|}~";
    int printable_len = strlen(ascii_printable);
	XGlyphInfo extents;

	XftTextExtentsUtf8( display, 
                        font->xft_font,
                        (const FcChar8 *) ascii_printable,
                        printable_len, 
                        &extents);

	font->height = font->xft_font->ascent + font->xft_font->descent;
    font->width = (((extents.xOff) + ((printable_len) - 1)) / (printable_len));

    return font;

fail:
    return NULL;
}

void font_destroy(TFont* font){
    free(font);
}
