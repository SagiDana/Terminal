#include "terminal.h"

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

    ret = XParseColor(terminal.display, terminal.colormap, background_color, &terminal.background_color);
    ASSERT(ret, "failed to parse color.\n");

    ret = XAllocColor(terminal.display, terminal.colormap, &terminal.background_color);
    ASSERT(ret, "failed to alloc color.\n");

    return 0;

fail:
    return -1;
}

int destroy_colors(){
    XFreeColors(terminal.display, 
                terminal.colormap,
                &terminal.background_color.pixel,
                1,
                0);

    return 0;
}

int end(){
    destroy_colors();

    return 0;
}

int start(){
    // create connection to the x server
    terminal.display = XOpenDisplay(NULL);
    ASSERT(terminal.display, "failed to open dispaly.\n");

    terminal.screen = XDefaultScreen(terminal.display);
    terminal.visual = XDefaultVisual(terminal.display, terminal.screen);
    terminal.colormap = XDefaultColormap(terminal.display, terminal.screen);

    setup_colors();

    terminal.x = 0;
    terminal.y = 0;
    terminal.width = 100;
    terminal.height = 40;

    Window parent;
    XSetWindowAttributes attrs;
    attrs.background_pixel = terminal.background_color.pixel;
    attrs.border_pixel = terminal.background_color.pixel;
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
    attrs.colormap = terminal.colormap;

    parent = XRootWindow(terminal.display, terminal.screen);

    terminal.window = XCreateWindow(terminal.display,
                                    parent,
                                    terminal.x,
                                    terminal.y,
                                    terminal.width,
                                    terminal.height,
                                    0,
                                    XDefaultDepth(terminal.display, terminal.screen), 
                                    InputOutput,
                                    terminal.visual,
                                    CWBitGravity | CWEventMask | CWColormap | CWBackPixel | CWBorderPixel,
                                    &attrs);

    XMapWindow(terminal.display, terminal.window);
    XSync(terminal.display, FALSE);

    terminal.xft_font = XftFontOpen(terminal.display,
                                    terminal.screen,
                                    XFT_FAMILY, XftTypeString, "ubuntu",
                                    XFT_SIZE, XftTypeDouble, 12.0,
                                    NULL);

    terminal.xft_draw = XftDrawCreate(  terminal.display,
                                        terminal.window,
                                        terminal.visual,
                                        terminal.colormap);

    XRenderColor xrcolor;
    XftColor	 xftcolor;
    xrcolor.red = 65535;
    xrcolor.green = 65535;
    xrcolor.blue = 65535;
    xrcolor.alpha = 65535;

    XftColorAllocValue( terminal.display,
                        terminal.visual,
                        terminal.colormap,
                        &xrcolor,
                        &xftcolor);

    unsigned char string_to_draw[] = "string to draw";
    XftDrawString8( terminal.xft_draw,
                    &xftcolor,
                    terminal.xft_font,
                    20, 50,
                    string_to_draw,
                    sizeof(string_to_draw) - 1);

    XFlush(terminal.display);

    return 0;

fail:
    return -1;
}

int run(){
    XEvent event;

    while (TRUE){
        while(XPending(terminal.display)){
            XNextEvent(terminal.display, &event);

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

