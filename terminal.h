#ifndef TERMINAL_H
#define TERMINAL_H


typedef struct{
    int cols_number;
    int raws_number;
}Terminal;


Terminal* terminal_create();
void terminal_destroy(Terminal* terminal);

#endif
