#include "t.h"


void on_event(XEvent* event){
    
}

static void (*event_handlers[LASTEvent])(XEvent*) = {
    [KeyPress] = on_event,
    [ClientMessage] = on_event,
    [ConfigureNotify] = on_event,
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

    ret = XAllocColor(xterminal.display, xterminal.colormap, &xterminal.background_color);
    ASSERT(ret, "failed to alloc color.\n");

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

    return 0;
}

int end(){
    pty_destroy(xterminal.pty);
    destroy_colors();

    return 0;
}

int draw_example(){
    xterminal.xft_font = XftFontOpen(   xterminal.display,
                                        xterminal.screen,
                                        XFT_FAMILY, XftTypeString, "ubuntu",
                                        XFT_SIZE, XftTypeDouble, 12.0,
                                        NULL);

    xterminal.xft_draw = XftDrawCreate( xterminal.display,
                                        xterminal.window,
                                        xterminal.visual,
                                        xterminal.colormap);

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

    unsigned char string_to_draw[] = "string to draw";
    XftDrawString8( xterminal.xft_draw,
                    &xftcolor,
                    xterminal.xft_font,
                    20, 50,
                    string_to_draw,
                    sizeof(string_to_draw) - 1);

    XFlush(xterminal.display);

    return 0;
}

int start(){
    // create connection to the x server
    xterminal.display = XOpenDisplay(NULL);
    ASSERT(xterminal.display, "failed to open dispaly.\n");

    xterminal.screen = XDefaultScreen(xterminal.display);
    xterminal.visual = XDefaultVisual(xterminal.display, xterminal.screen);
    xterminal.colormap = XDefaultColormap(xterminal.display, xterminal.screen);

    setup_colors();

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
    XEvent event;

    while (TRUE){
        while(XPending(xterminal.display)){
            XNextEvent(xterminal.display, &event);

            if (event_handlers[event.type]){
                (event_handlers[event.type])(&event);
            }
        }
    }

    return 0;
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

