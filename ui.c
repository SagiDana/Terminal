#include "ui.h"
#include "color.h"

#include <time.h>


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
        if ((terminal_keys[i].mod != XK_ANY_MOD) &&
            (ONLY_MODIFIERS(event->state) != ONLY_MODIFIERS(terminal_keys[i].mod))){
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

int draw();
int clean_screen();

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

    int cols_number = (xterminal.width - (2 * border_pixels)) / xterminal.font->width;
    int rows_number = (xterminal.height - (2 * border_pixels)) / xterminal.font->height;

    // no need to resize!
    if (cols_number == xterminal.terminal->cols_number &&
        rows_number == xterminal.terminal->rows_number){
        return;
    }

    LOG("resize just happened.\n");

	XFreePixmap(xterminal.display, xterminal.drawable);

	xterminal.drawable = XCreatePixmap( xterminal.display, 
                                        xterminal.window, 
                                        xterminal.width,
                                        xterminal.height,
                                        DefaultDepth(xterminal.display, xterminal.screen));

	XftDrawChange(xterminal.xft_draw, xterminal.drawable);

    clean_screen();

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

void on_expose(XEvent* event){
    draw();
}

static void (*event_handlers[LASTEvent])(XEvent*) = {
    [KeyPress] = on_key_press,
    [ClientMessage] = on_event,
    [ConfigureNotify] = on_configure_notify,
    [VisibilityNotify] = on_event,
    [UnmapNotify] = on_event,
    [Expose] = on_expose,
    [FocusIn] = on_event,
    [FocusOut] = on_event,
    [MotionNotify] = on_event,
    [ButtonPress] = on_event,
    [ButtonRelease] = on_event,
    [SelectionNotify] = on_event,
    [PropertyNotify] = on_event,
    [SelectionRequest] = on_event
};

// ------------------------------------------------------------------------------------

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

    if (element_foreground_color < 16){
        current_foreground_color = map_4bit_to_true_color(element_foreground_color);
    }else if (element_foreground_color < 256){
        current_foreground_color = get_xterm_color(element_foreground_color);
    }else{
        current_foreground_color = element_foreground_color;
    }

    current_background_color = map_4bit_to_true_color(element_background_color);
    if (current_background_color == 0){
        current_background_color = element_background_color;
    }

    if (element_background_color < 16){
        current_background_color = map_4bit_to_true_color(element_background_color);
    }else if (element_background_color < 256){
        current_background_color = get_xterm_color(element_background_color);
    }else{
        current_background_color = element_background_color;
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

        draw_x = border_pixels + (x * xterminal.font->width);
        draw_y = border_pixels + (y * xterminal.font->height);

        // draw background
        XftDrawRect(xterminal.xft_draw, 
                &xft_background_color, 
                draw_x,
                draw_y,
                xterminal.font->width,
                xterminal.font->height);

        /* Set the clip region because Xft is sometimes dirty. */
        XRectangle rectangle;
        rectangle.x = 0;
        rectangle.y = 0;
        rectangle.height = xterminal.font->height;
        rectangle.width = xterminal.font->width;
        XftDrawSetClipRectangles(xterminal.xft_draw, draw_x, draw_y, &rectangle, 1);

        xft_glyph_spec.font = current_font;
        xft_glyph_spec.glyph = glyph_index;
        xft_glyph_spec.x = draw_x;
        // it is important to add the ascent  to the y.
        xft_glyph_spec.y = draw_y + xterminal.font->normal_font->ascent;

        // draw foreground
        XftDrawGlyphFontSpec(   xterminal.xft_draw, 
                                &xft_foreground_color, 
                                &xft_glyph_spec, 
                                1);

        /* Reset clip to none. */
        XftDrawSetClip(xterminal.xft_draw, 0);
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
    XRectangle rectangle;
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.height = xterminal.height;
    rectangle.width = xterminal.width;
    XftDrawSetClipRectangles(xterminal.xft_draw, 0, 0, &rectangle, 1);

    XftDrawRect(    xterminal.xft_draw,
                    &xterminal.background_color,
                    0,
                    0,
                    xterminal.width,
                    xterminal.height);

    /* Reset clip to none. */
    XftDrawSetClip(xterminal.xft_draw, 0);

    // all drawing takes effect here.
	XCopyArea(  xterminal.display, 
                xterminal.drawable,
                xterminal.window,
                xterminal.gc,
                0, 0,
                xterminal.width,
                xterminal.height,
                0, 0);

    XFlush(xterminal.display);
    return 0;
}

int draw(){
    int ret;
    int x,y;

    for (y = 0; y < xterminal.terminal->rows_number; y++){
        for (x = 0; x < xterminal.terminal->cols_number; x++){
            TElement* element = terminal_element(xterminal.terminal, x, y);
            if (element->dirty == 1){
                ret = draw_element(element, x, y);
                ASSERT(ret == 0, "failed to draw element.\n");
                element->dirty = 0;
            }
            // manual set dirty if cursor.
            if (x == xterminal.terminal->cursor.x &&
                    y == xterminal.terminal->cursor.y){
                ret = draw_element(element, x, y);
                ASSERT(ret == 0, "failed to draw element.\n");
                element->dirty = 1;
            }
        }
    }

    // all drawing takes effect here.
	XCopyArea(  xterminal.display, 
                xterminal.drawable,
                xterminal.window,
                xterminal.gc,
                0, 0,
                xterminal.width,
                xterminal.height,
                0, 0);

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
    xterminal.width = (border_pixels * 2) + xterminal.font->width * cols;
    xterminal.height = (border_pixels * 2) + xterminal.font->height * rows;

    char* args[] = { shell, NULL };
    xterminal.pty = pty_create(args, terminal_name);
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

	XGCValues gcvalues;
	memset(&gcvalues, 0, sizeof(gcvalues));
	gcvalues.graphics_exposures = FALSE;

	xterminal.gc = XCreateGC(   xterminal.display, 
                                parent, 
                                GCGraphicsExposures,
                                &gcvalues);

	xterminal.drawable = XCreatePixmap( xterminal.display, 
                                        xterminal.window, 
                                        xterminal.width,
                                        xterminal.height,
                                        DefaultDepth(xterminal.display, xterminal.screen));

	XSetForeground( xterminal.display, 
                    xterminal.gc, 
                    xterminal.background_color.pixel);

	XFillRectangle( xterminal.display, 
                    xterminal.drawable, 
                    xterminal.gc, 
                    0, 
                    0, 
                    xterminal.width, 
                    xterminal.height);

    xterminal.xft_draw = XftDrawCreate( xterminal.display,
                                        xterminal.drawable,
                                        xterminal.visual,
                                        xterminal.colormap);
    ASSERT((xterminal.xft_draw != NULL), "failed to create xft_draw\n");

    XMapWindow(xterminal.display, xterminal.window);
    XSync(xterminal.display, FALSE);

    return 0;

fail:
    return -1;
}

int is_xevent_pending(){
    // wait;
    int ret; 
    int x_fd = XConnectionNumber(xterminal.display);
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(x_fd, &read_fds);

    struct timespec x_timeout;
    x_timeout.tv_sec = 0;
    x_timeout.tv_nsec = (1000 * 1E6) / 120; // 120 fps

    ret = pselect(x_fd + 1, &read_fds, NULL, NULL, &x_timeout, NULL);

    return ret > 0;
}

int run(){
    int ret;
    int to_draw = FALSE;
    XEvent event;

	/* Waiting for window mapping */
	do {
		XNextEvent(xterminal.display, &event);

        if (event_handlers[event.type]){
            (event_handlers[event.type])(&event);
        }
        XSync(xterminal.display, FALSE);
	} while (event.type != MapNotify);

    clean_screen();

    while (TRUE){
        if (is_xevent_pending()){
            while(XPending(xterminal.display)){
                XNextEvent(xterminal.display, &event);

                if (event_handlers[event.type]){
                    (event_handlers[event.type])(&event);
                }
                XSync(xterminal.display, FALSE);
            }
        }

        // waiting to either xevent is pending or pty has data.
        if (pty_pending(xterminal.pty)){
            ret = read_from_pty();
            ASSERT(ret == 0, "failed to read from pty.\n");
            to_draw = TRUE;
        }

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

