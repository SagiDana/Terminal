#include "terminal.h"
#include "common.h"

#include <stdlib.h>


Terminal* terminal_create(int cols_number, int rows_number){
    Terminal* terminal = NULL;

    terminal = (Terminal*) malloc(sizeof(Terminal));
    ASSERT(terminal, "failed to malloc() terminal.\n");

    terminal->cols_number = cols_number;
    terminal->rows_number = rows_number;

fail:
    return terminal;
}

void terminal_destroy(Terminal* terminal){
    free(terminal);
}
