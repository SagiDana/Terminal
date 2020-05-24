#include "font.h"

#include <stdlib.h>


// font styles definitions
#define STYLE_BOLD      (1 << 0)
#define STYLE_ITALIC    (1 << 1)

// font style operations
#define RESET_STYLE()    (style &= 0)
#define IS_STYLE(x)       (style & x)
#define SET_STYLE(x)      (style |= x)
#define SET_NO_STYLE(x)   (style &= (~x))


TFont* font_create(  Display* display,
                     int screen,
                     char* font_name, 
                     double font_size){

    TFont* font = NULL;
    unsigned int style;

    font = (TFont*) malloc(sizeof(TFont));
    ASSERT(font, "failed to malloc font.\n");

    RESET_STYLE();
    font->normal_font = font_get(display, screen, font_name, font_size, style);
    ASSERT(font->normal_font, "failed to get normal font.\n");

    SET_STYLE(STYLE_BOLD);
    font->bold_font = font_get(display, screen, font_name, font_size, style);
    ASSERT(font->bold_font, "failed to get bold font.\n");

    SET_STYLE(STYLE_ITALIC);
    font->italic_bold_font = font_get(display, screen, font_name, font_size, style);
    ASSERT(font->italic_bold_font, "failed to get italic bold font.\n");

    RESET_STYLE();
    SET_STYLE(STYLE_ITALIC);
    font->italic_font = font_get(display, screen, font_name, font_size, style);
    ASSERT(font->italic_font, "failed to get italic font.\n");

    // all printable characters
    char ascii_printable[] =    " !\"#$%&'()*+,-./0123456789:;<=>?"
                                "@abcdefghijklmnopqrstuvwxyz[\\]^_"
                                "`abcdefghijklmnopqrstuvwxyz{|}~";
    int printable_len = strlen(ascii_printable);
	XGlyphInfo extents;

	XftTextExtentsUtf8( display, 
                        font->normal_font,
                        (const FcChar8 *) ascii_printable,
                        printable_len, 
                        &extents);

	font->height = font->normal_font->ascent + font->normal_font->descent;
    font->width = (((extents.xOff) + ((printable_len) - 1)) / (printable_len));

    return font;

fail:
    return NULL;
}

void font_destroy(TFont* font){
    free(font);
}

XftFont* font_get( Display* display,
                   int screen,
                   char* font_name, 
                   double font_size,
                   unsigned int style){
    XftFont* found = NULL;
	FcPattern *pattern = NULL;
    FcResult result = 0;
	FcPattern *match = NULL;

    pattern = FcNameParse((FcChar8 *)font_name);
    ASSERT(pattern, "fontconfig failed to find font.\n");

    // reset size
    FcPatternDel(pattern, FC_PIXEL_SIZE);
    FcPatternDel(pattern, FC_SIZE);

    // setting our wanted size
    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, font_size);

    // resetin attributes.
	FcPatternDel(pattern, FC_SLANT);
	FcPatternDel(pattern, FC_WEIGHT);

    if (IS_STYLE(STYLE_BOLD)){
        FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
    }
    if (IS_STYLE(STYLE_ITALIC)){
        FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC);
    }
	// FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
	// FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN);


    // searching font with our size.
	match = FcFontMatch(NULL, pattern, &result);
    ASSERT_TO(fail_on_match, match, "fontconfig failed to find font.\n");

    found = XftFontOpenPattern(display, match);
    ASSERT_TO(fail_on_open, found, "xft failed to open match.\n");

    // found = XftFontOpen(display,
                        // screen,
                        // XFT_FAMILY, XftTypeString, font_name,
                        // XFT_SIZE, XftTypeDouble, font_size,
                        // NULL);

fail_on_open:
    FcPatternDestroy(match);
fail_on_match:
    FcPatternDestroy(pattern);
fail:
    return found;
}
