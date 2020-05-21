#ifndef TERMINAL_H
#define TERMINAL_H


typedef struct{
    int cols_number;
    int rows_number;
}Terminal;


Terminal* terminal_create(int cols_number, int rows_number);
void terminal_destroy(Terminal* terminal);

#endif
