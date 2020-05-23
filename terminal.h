#ifndef TERMINAL_H
#define TERMINAL_H

#include "element.h"

#define CSI_MAX_PARAMETERS_CHARS (16)

typedef struct{
    int x;
    int y;
}TCursor;

typedef struct{
    int cols_number;
    int rows_number;

    Element* screen;
    TCursor cursor;

    int start_line_index;

    unsigned int mode;

    // 16 max number of chars for parameters.
    unsigned char csi_parameters[CSI_MAX_PARAMETERS_CHARS + 1]; 
    int csi_parameters_index;
}Terminal;


Terminal* terminal_create(int cols_number, int rows_number);
void terminal_destroy(Terminal* terminal);

int terminal_resize(Terminal* terminal, 
                    int cols_number, 
                    int rows_number);

int terminal_forward_cursor(Terminal* terminal);
int terminal_new_line(Terminal* terminal);
int terminal_empty_line(Terminal* terminal, int y);

int terminal_emulate(Terminal* terminal, unsigned int character_code);

int terminal_push(Terminal* terminal, char* buf, int len);
Element* terminal_element(Terminal* terminal, int x, int y);

int terminal_delete_element(Terminal* terminal, int x, int y);

#endif
