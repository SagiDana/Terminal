#ifndef TERMINAL_H
#define TERMINAL_H

#include "element.h"


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
}Terminal;


Terminal* terminal_create(int cols_number, int rows_number);
void terminal_destroy(Terminal* terminal);

int terminal_resize(Terminal* terminal, 
                    int cols_number, 
                    int rows_number);

int terminal_forward_cursor(Terminal* terminal);
int terminal_push(Terminal* terminal, char* buf, int len);
Element* terminal_element(Terminal* terminal, int x, int y);

#endif
