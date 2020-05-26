#ifndef TERMINAL_H
#define TERMINAL_H

#include "element.h"
#include "pty.h"


// attributes definitions
#define BOLD_ATTR           (1 << 0)
#define UNDERSCORE_ATTR     (1 << 1)
#define BLINK_ATTR          (1 << 2)
#define UNDERLINE_ATTR      (1 << 3)
#define REVERSE_ATTR        (1 << 4)

typedef struct{
    int x;
    int y;
}TCursor;

#define OSC_MAX_CHARS (100)
#define CSI_MAX_PARAMETERS_CHARS (16)
typedef struct{
    int cols_number;
    int rows_number;

    TElement* screen;
    TCursor cursor;

    int start_line_index;

    int top;
    int bottom;

    unsigned int default_background_color;
    unsigned int default_foreground_color;

    TPty* pty; // the attached pty.

    // ---- parameters to keep state of control codes! ----

    unsigned int mode;
    unsigned int attributes;
    unsigned int background_color;
    unsigned int foreground_color;

    unsigned char csi_parameters[CSI_MAX_PARAMETERS_CHARS + 1]; 
    int csi_parameters_index;

    unsigned char osc_buffer[OSC_MAX_CHARS + 1];
    int osc_buffer_index;
}Terminal;


Terminal* terminal_create(  TPty* pty,
                            int cols_number, 
                            int rows_number, 
                            char* background_color,
                            char* foreground_color);
void terminal_destroy(Terminal* terminal);

int terminal_resize(Terminal* terminal, 
                    int cols_number, 
                    int rows_number);

int terminal_forward_cursor(Terminal* terminal);
int terminal_new_line(Terminal* terminal);

int terminal_empty_element(Terminal* terminal, int x, int y);
int terminal_empty_line(Terminal* terminal, int y);
int terminal_empty(Terminal* terminal);

int terminal_scrolldown(Terminal* terminal, int y, int lines_number);
int terminal_move_line(Terminal* terminal, int src_y, int dst_y);

int terminal_emulate(Terminal* terminal, unsigned int character_code);

int terminal_push(Terminal* terminal, char* buf, int len);
TElement* terminal_element(Terminal* terminal, int x, int y);


#endif
