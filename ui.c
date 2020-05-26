#include "ui.h"
#include "color.h"


// ------------------------------------------------------------------------------------
// helper functions
// ------------------------------------------------------------------------------------

Key* get_key_by_event(Display* display, XKeyEvent* event){
#define ONLY_MODIFIERS(x) (x & (ShiftMask   | \
                                ControlMask | \
                                Mod1Mask    | \
                                Mod2Mask    | \
                                Mod3Mask    | \
                                Mod4Mask    | \
                                Mod5Mask))
    int i;
    for (i = 0; i < LENGTH(terminal_keys); i++){
        if (event->keycode != XKeysymToKeycode(display, terminal_keys[i].keysym)){
            continue;
        }
        if (ONLY_MODIFIERS(event->state) != ONLY_MODIFIERS(terminal_keys[i].mod)){
            continue;
        }

        return &terminal_keys[i];
    }
    return NULL;
}

int read_from_pty(){
    int ret;
    char buf[4096];
    int bytes_read = 1;

    memset(buf, 0, sizeof(buf));

    bytes_read = pty_read(xterminal.pty, buf, sizeof(buf));
    ASSERT((bytes_read >= 0), "failed to read from pty.\n");

    ret = terminal_push(xterminal.terminal, buf, bytes_read);
    ASSERT((ret == 0), "failed to push to terminal.\n");

    return 0;

fail:
    return -1;
}

// ------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------
// on event handlers
// ------------------------------------------------------------------------------------

void on_event(XEvent* event){
}

void on_configure_notify(XEvent* event){
    int ret;
    XConfigureEvent* configure_event = &event->xconfigure;

    xterminal.width = configure_event->width;
    xterminal.height = configure_event->height;

    int cols_number = configure_event->width / xterminal.font->width;
    int rows_number = configure_event->height / xterminal.font->height;

    // no need to resize!
    if (cols_number == xterminal.terminal->cols_number &&
        rows_number == xterminal.terminal->rows_number){
        return;
    }

    LOG("resize just happened.\n");

	// XFreePixmap(xterminal.display, xterminal.drawable);

	// xterminal.drawable = XCreatePixmap( xterminal.display, 
                                        // xterminal.window, 
                                        // xterminal.width,
                                        // xterminal.height,
                                        // DefaultDepth(xterminal.display, xterminal.screen));

	// XftDrawChange(xterminal.xft_draw, xterminal.drawable);

    ret = terminal_resize(xterminal.terminal, cols_number, rows_number);
    ASSERT(ret == 0, "failed to resize terminal.\n");

fail:
    return;
}

void on_key_press(XEvent* event){
    int len;
    int ret;
    char buf[64];
    char* string = NULL;
	KeySym keysym;
	XKeyEvent *key_event = &event->xkey;

    Key* special_key = get_key_by_event(xterminal.display, key_event);
    if (special_key){
        string = special_key->string;
        len = strlen(string);

    }else{
        // getting string representation of the key that just pressed.
        len = XLookupString(key_event, buf, sizeof(buf), &keysym, NULL);
        ASSERT((len > 0), "XLookupString could not find string to key press.\n");
        string = buf;
    }

    // write key pressed to pty.
    ret = pty_write(xterminal.pty, string, len);
    ASSERT((ret >= 0), "failed to pty write on key press!\n");

fail:
    return;
}

// ------------------------------------------------------------------------------------

static void (*event_handlers[LASTEvent])(XEvent*) = {
    [KeyPress] = on_key_press,
    [ClientMessage] = on_event,
    [ConfigureNotify] = on_configure_notify,
    [VisibilityNotify] = on_event,
    [UnmapNotify] = on_event,
    [Expose] = on_event,
    [FocusIn] = on_event,
    [FocusOut] = on_event,
    [MotionNotify] = on_event,
    [ButtonPress] = on_event,
    [ButtonRelease] = on_event,
    [SelectionNotify] = on_event,
    [PropertyNotify] = on_event,
    [SelectionRequest] = on_event
};

int setup_colors(){
    int ret;

	ret = XftColorAllocName(    xterminal.display,
                                xterminal.visual,
                                xterminal.colormap,
                                background_color,
                                &xterminal.background_color);
    ASSERT(ret, "failed setting up colors.\n");

	ret = XftColorAllocName(    xterminal.display,
                                xterminal.visual,
                                xterminal.colormap,
                                foreground_color,
                                &xterminal.foreground_color);
    ASSERT(ret, "failed setting up colors.\n");

    return 0;

fail:
    return -1;
}

int setup_fonts(){
    xterminal.font = font_create(   xterminal.display,
                                    xterminal.screen,
                                    font_name,
                                    font_size);
    ASSERT(xterminal.font, "failed to create font\n");
    return 0;

fail:
    return -1;
}

int destroy_colors(){
    XftColorFree(   xterminal.display,
                    xterminal.visual,
                    xterminal.colormap,
                    &xterminal.background_color);

    XftColorFree(   xterminal.display,
                    xterminal.visual,
                    xterminal.colormap,
                    &xterminal.foreground_color);

    return 0;
}

int destroy_fonts(){
    font_destroy(xterminal.font);
    return 0;
}

int end(){
    pty_destroy(xterminal.pty);
    destroy_colors();
    destroy_fonts();
    terminal_destroy(xterminal.terminal);

    return 0;
}

int draw_element(TElement* element, int x, int y){
    int ret;
    XftGlyphFontSpec xft_glyph_spec;
    XftColor xft_foreground_color;
    XftColor xft_background_color;
    unsigned int current_foreground_color = 0;
    unsigned int current_background_color = 0;

    // create the colors
    // XColor
    unsigned int element_foreground_color = element->foreground_color;
    unsigned int element_background_color = element->background_color;

    if ((x == xterminal.terminal->cursor.x) &&
        (y == xterminal.terminal->cursor.y)){
        element_foreground_color = element->background_color;
        element_background_color = element->foreground_color;
    }

    if (element->attributes & REVERSE_ATTR){
        element_foreground_color = element->background_color;
        element_background_color = element->foreground_color;
    }

	XRenderColor color = { .alpha = 0xffff };
    current_foreground_color = map_4bit_to_true_color(element_foreground_color);
    if (current_foreground_color == 0){
        current_foreground_color = element_foreground_color;
    }
    color.red = TRUE_COLOR_RED_16BIT(current_foreground_color);
    color.green = TRUE_COLOR_GREEN_16BIT(current_foreground_color);
    color.blue = TRUE_COLOR_BLUE_16BIT(current_foreground_color);

    ret = XftColorAllocValue(   xterminal.display, 
                                xterminal.visual,
                                xterminal.colormap, 
                                &color, 
                                &xft_foreground_color);
    ASSERT(ret != 0, "failed to allocate color.\n");

    current_background_color = map_4bit_to_true_color(element_background_color);
    if (current_background_color == 0){
        current_background_color = element_background_color;
    }
    color.red = TRUE_COLOR_RED_16BIT(current_background_color);
    color.green = TRUE_COLOR_GREEN_16BIT(current_background_color);
    color.blue = TRUE_COLOR_BLUE_16BIT(current_background_color);

    ret = XftColorAllocValue(   xterminal.display, 
                                xterminal.visual,
                                xterminal.colormap, 
                                &color, 
                                &xft_background_color);
    ASSERT(ret != 0, "failed to allocate color.\n");

    XftFont* current_font = xterminal.font->normal_font;
    if (element->attributes & BOLD_ATTR){
        current_font = xterminal.font->bold_font;
    }

    // create the glyph font spec
    FT_UInt glyph_index = XftCharIndex( xterminal.display, 
                                        current_font,
                                        element->character_code);

    if (glyph_index){
        int draw_x, draw_y;

        draw_x = x * xterminal.font->width;
        draw_y = y * xterminal.font->height;

        // draw background
        XftDrawRect(xterminal.xft_draw, 
                &xft_background_color, 
                draw_x,
                draw_y,
                xterminal.font->width,
                xterminal.font->height);

        xft_glyph_spec.font = current_font;
        xft_glyph_spec.glyph = glyph_index;
        xft_glyph_spec.x = draw_x;

        // TODO: find out why this happens. and fix it!
        xft_glyph_spec.y = (y + 1) * (xterminal.font->height); 

        // draw foreground
        XftDrawGlyphFontSpec(   xterminal.xft_draw, 
                                &xft_foreground_color, 
                                &xft_glyph_spec, 
                                1);

    }

    XftColorFree(   xterminal.display,
                    xterminal.visual,
                    xterminal.colormap,
                    &xft_foreground_color);

    XftColorFree(   xterminal.display,
                    xterminal.visual,
                    xterminal.colormap,
                    &xft_background_color);

    return 0;

fail:
    return -1;
}

int clean_screen(){
    XftDrawRect(    xterminal.xft_draw,
                    &xterminal.background_color,
                    0,
                    0,
                    xterminal.width,
                    xterminal.height);


    XFlush(xterminal.display);

    return 0;
}

int draw(){
    int ret;
    int x,y;

    clean_screen();

    for (y = 0; y < xterminal.terminal->rows_number; y++){
        for (x = 0; x < xterminal.terminal->cols_number; x++){
            TElement* element = terminal_element(xterminal.terminal, x, y);
            ret = draw_element(element, x, y);
            ASSERT(ret == 0, "failed to draw element.\n");
        }
    }
    XFlush(xterminal.display);

    return 0;
fail:
    return -1;
}

int start(){
    // create connection to the x server
    xterminal.display = XOpenDisplay(NULL);
    ASSERT(xterminal.display, "failed to open dispaly.\n");

    xterminal.screen = XDefaultScreen(xterminal.display);
    xterminal.visual = XDefaultVisual(xterminal.display, xterminal.screen);
    xterminal.colormap = XDefaultColormap(xterminal.display, xterminal.screen);

    setup_colors();
    setup_fonts();

    xterminal.x = 0;
    xterminal.y = 0;
    xterminal.width = xterminal.font->width * cols;
    xterminal.height = xterminal.font->height * rows;

    char* args[] = { shell, NULL };
    xterminal.pty = pty_create(args);
    ASSERT(xterminal.pty, "failed creating pty.\n");

    xterminal.terminal = terminal_create(   xterminal.pty,
                                            xterminal.width, 
                                            xterminal.height,
                                            background_color,
                                            foreground_color);
    ASSERT(xterminal.terminal, "failed to create terminal.\n");

    Window parent;
    XSetWindowAttributes attrs;
    attrs.background_pixel = xterminal.background_color.pixel;
    attrs.border_pixel = xterminal.background_color.pixel;
    attrs.bit_gravity = NorthWestGravity;
    attrs.event_mask =  FocusChangeMask | 
                        KeyPressMask | 
                        KeyReleaseMask | 
                        ExposureMask |
                        VisibilityChangeMask |
                        StructureNotifyMask |
                        ButtonMotionMask | 
                        ButtonPressMask | 
                        ButtonReleaseMask;
    attrs.colormap = xterminal.colormap;

    parent = XRootWindow(xterminal.display, xterminal.screen);

    xterminal.window = XCreateWindow(xterminal.display,
                                    parent,
                                    xterminal.x,
                                    xterminal.y,
                                    xterminal.width,
                                    xterminal.height,
                                    0,
                                    DefaultDepth(xterminal.display, xterminal.screen), 
                                    InputOutput,
                                    xterminal.visual,
                                    CWBitGravity | CWEventMask | CWColormap | CWBackPixel | CWBorderPixel,
                                    &attrs);

	// xterminal.drawable = XCreatePixmap( xterminal.display, 
                                        // xterminal.window, 
                                        // xterminal.width,
                                        // xterminal.height,
                                        // DefaultDepth(xterminal.display, xterminal.screen));
    // ASSERT((xterminal.drawable != BadValue), "failed to create pixmap\n");
    // ASSERT((xterminal.drawable != BadDrawable), "failed to create pixmap\n");
    // ASSERT((xterminal.drawable != BadAlloc), "failed to create pixmap\n");

	// XGCValues gcvalues;
	// memset(&gcvalues, 0, sizeof(gcvalues));
	// gcvalues.graphics_exposures = FALSE;

	// GC gc = XCreateGC(  xterminal.display, 
                        // parent, 
                        // GCGraphicsExposures,
			            // &gcvalues);

	// XSetForeground( xterminal.display, 
                    // gc, 
                    // xterminal.background_color.pixel);

	// XFillRectangle( xterminal.display, 
                    // xterminal.drawable, 
                    // gc, 
                    // 0, 
                    // 0, 
                    // xterminal.width, 
                    // xterminal.height);

    // xterminal.xft_draw = XftDrawCreate( xterminal.display,
                                        // xterminal.drawable,
                                        // xterminal.visual,
                                        // xterminal.colormap);

    xterminal.xft_draw = XftDrawCreate( xterminal.display,
                                        xterminal.window,
                                        xterminal.visual,
                                        xterminal.colormap);

    XMapWindow(xterminal.display, xterminal.window);
    XSync(xterminal.display, FALSE);

    return 0;

fail:
    return -1;
}

int run(){
    int ret;
    int to_draw = FALSE;
    XEvent event;

    while (TRUE){
        if (pty_pending(xterminal.pty)){
            ret = read_from_pty();
            ASSERT(ret == 0, "failed to read from pty.\n");
            to_draw = TRUE;
        }

        while(XPending(xterminal.display)){
            XNextEvent(xterminal.display, &event);

            if (event_handlers[event.type]){
                (event_handlers[event.type])(&event);
            }
            XSync(xterminal.display, FALSE);
        }
        // LOG("finished X even loop.\n");

        if (to_draw){
            ret = draw();
            ASSERT(ret == 0, "failed to draw.\n");

            XSync(xterminal.display, FALSE);

            to_draw = FALSE;
        }
    }
    return 0;

fail:
    return -1;
}

int main(){
    int ret;

    LOG("terminal has started.\n");
    ret = start();
    ASSERT(ret == 0, "failed to start terminal.\n");

    ret = run();
    ASSERT(ret == 0, "failed to run terminal.\n");

    ret = end();
    ASSERT(ret == 0, "failed to end terminal properly.\n");
    LOG("terminal has finished.\n");
    return 0;

fail:
    return -1;
}
