#include "terminal.h"

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>


int setup(){
    // create connection to the x server
    terminal.display = XOpenDisplay(NULL);
    ASSERT(terminal.display, "failed to open dispaly.\n");

    terminal.screen = XDefaultScreen(terminal.display);
    terminal.visual = XDefaultVisual(terminal.display, terminal.screen);
    terminal.colormap = XDefaultColormap(terminal.display, terminal.screen);

    terminal.x = 0;
    terminal.y = 0;
    terminal.width = 100;
    terminal.height = 40;

    Window parent;
    XSetWindowAttributes attrs;
    // attrs.background_pixel = ;
    // attrs.border_pixel = ;
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
                                    CWBitGravity | CWEventMask | CWColormap,
                                    &attrs);
    XMapWindow(terminal.display, terminal.window);
    XSync(terminal.display, FALSE);

    return 0;

fail:
    return -1;
}

int run(){

}

int main(){
    int ret;

    LOG("terminal has started.\n");
    ret = setup();
    ASSERT(ret == 0, "failed to setup terminal\n");

    LOG("terminal has finished.\n");
    return 0;

fail:
    return -1;
}

