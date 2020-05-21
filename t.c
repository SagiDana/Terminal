#include "t.h"


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

    LOG("x: %d, y: %d, width: %d, height: %d\n", configure_event->x,
                                                 configure_event->y,
                                                 configure_event->width,
                                                 configure_event->height);

    int cols_number = configure_event->width / xterminal.font->width;
    int rows_number = configure_event->height / xterminal.font->height;

    ret = terminal_resize(xterminal.terminal, cols_number, rows_number);
    ASSERT(ret == 0, "failed to resize terminal.\n");

fail:
    return;
}

void on_key_press(XEvent* event){
    int len;
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

    LOG("key pressed: %s\n", string);

    // write key pressed to pty.
    pty_write(xterminal.pty, string, len);

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

    ret = XParseColor(xterminal.display, xterminal.colormap, background_color, &xterminal.background_color);
    ASSERT(ret, "failed to parse color.\n");
    ret = XParseColor(xterminal.display, xterminal.colormap, foreground_color, &xterminal.foreground_color);
    ASSERT(ret, "failed to parse color.\n");

    ret = XAllocColor(xterminal.display, xterminal.colormap, &xterminal.background_color);
    ASSERT(ret, "failed to alloc color.\n");

    ret = XAllocColor(xterminal.display, xterminal.colormap, &xterminal.foreground_color);
    ASSERT(ret, "failed to alloc color.\n");

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
    XFreeColors(xterminal.display, 
                xterminal.colormap,
                &xterminal.background_color.pixel,
                1,
                0);
    XFreeColors(xterminal.display, 
                xterminal.colormap,
                &xterminal.foreground_color.pixel,
                1,
                0);

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

    return 0;
}

int draw_element(Element* element, int x, int y){
    XRenderColor xrcolor;
    XftColor	 xftcolor;
    xrcolor.red = 65535;
    xrcolor.green = 65535;
    xrcolor.blue = 65535;
    xrcolor.alpha = 65535;

    XftColorAllocValue( xterminal.display,
                        xterminal.visual,
                        xterminal.colormap,
                        &xrcolor,
                        &xftcolor);

    unsigned char element_as_string = (unsigned char) element->character_code;

    // do not draw empty elements.
    if (element_as_string == 0){
        return 0;
    }

    XftDrawString8( xterminal.xft_draw,
                    &xftcolor,
                    xterminal.font->xft_font,
                    (x * xterminal.font->width),
                    (y * xterminal.font->height),
                    &element_as_string,
                    1);

    return 0;
}

int draw(){
    int ret;
    int x,y;
    for (x = 0; x < xterminal.terminal->cols_number; x++){
        for (y = 0; y < xterminal.terminal->rows_number; y++){
            Element* element = terminal_element(xterminal.terminal, x, y);
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
    xterminal.width = cols;
    xterminal.height = rows;

    xterminal.terminal = terminal_create(xterminal.width, xterminal.height);
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
                                    XDefaultDepth(xterminal.display, xterminal.screen), 
                                    InputOutput,
                                    xterminal.visual,
                                    CWBitGravity | CWEventMask | CWColormap | CWBackPixel | CWBorderPixel,
                                    &attrs);

    xterminal.xft_draw = XftDrawCreate( xterminal.display,
                                        xterminal.window,
                                        xterminal.visual,
                                        xterminal.colormap);

    XMapWindow(xterminal.display, xterminal.window);
    XSync(xterminal.display, FALSE);

    char* args[] = { shell, NULL };
    xterminal.pty = pty_create(args);
    ASSERT(xterminal.pty, "failed creating pty.\n");

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
            LOG("pty pending!\n");

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

        if (to_draw){
            ret = draw();
            ASSERT(ret == 0, "failed to draw.\n");
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

