#include "terminal.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>


#define ELEMENT_X (terminal->cursor.x)
#define ELEMENT_Y ((terminal->start_line_index + terminal->cursor.y) % terminal->rows_number)
#define ELEMENT (terminal->screen[(ELEMENT_Y * terminal->cols_number) + ELEMENT_X])

Terminal* terminal_create(int cols_number, int rows_number){
    Terminal* terminal = NULL;

    terminal = (Terminal*) malloc(sizeof(Terminal));
    ASSERT(terminal, "failed to malloc() terminal.\n");
    memset(terminal, 0, sizeof(Terminal));

    terminal->cols_number = cols_number;
    terminal->rows_number = rows_number;

    terminal->start_line_index = 0;

    // start position.
    terminal->cursor.x = 0;
    terminal->cursor.y = 0;

    terminal->screen = (Element*) malloc(sizeof(Element) * cols_number * rows_number);
    ASSERT_TO(fail_on_screen, terminal->screen, "failed to malloc screen.\n");
    memset(terminal->screen, 0, sizeof(Element) * cols_number * rows_number);

    return terminal;

fail_on_screen:
    free(terminal);
fail:
    return NULL;
}

void terminal_destroy(Terminal* terminal){
    ASSERT(terminal, "trying to destroy NULL terminal.\n");
    ASSERT(terminal->screen, "trying to destroy NULL terminal->screen.\n");
    
    free(terminal->screen);
    free(terminal);

fail:
    return;
}

/*
 * current implementation is reseting 
 * the terminal completely for now.. 
 * TODO:
 */
int terminal_resize(Terminal* terminal, int cols_number, int rows_number){

    free(terminal->screen);

    terminal->cols_number = cols_number;
    terminal->rows_number = rows_number;
    terminal->cursor.x = 0;
    terminal->cursor.y = 0;
    terminal->start_line_index = 0;

    terminal->screen = (Element*) malloc(sizeof(Element) * cols_number * rows_number);
    ASSERT(terminal->screen, "failed to malloc screen.\n");
    memset(terminal->screen, 0, sizeof(Element) * cols_number * rows_number);

    return 0;

fail:
    return -1;
}

int terminal_rotate_lines(Terminal* terminal){
    terminal->start_line_index = (terminal->start_line_index + 1) % terminal->rows_number;

    return 0;
}

int terminal_forward_cursor(Terminal* terminal){
    if (terminal->cursor.x < terminal->cols_number){
        terminal->cursor.x++;
        return 0;
    }

    return terminal_new_line(terminal);
}

int terminal_empty_line(Terminal* terminal, int y){
    int real_y = ((terminal->start_line_index + y) % terminal->rows_number);
    memset( &terminal->screen[real_y * terminal->cols_number],
            0,
            sizeof(Element) * terminal->cols_number);

    return 0;
}

int terminal_new_line(Terminal* terminal){
    int ret;

    if (terminal->cursor.y + 1 < terminal->rows_number){
        terminal->cursor.y++;
    }else{
        ret = terminal_rotate_lines(terminal);
        ASSERT(ret == 0, "failed to rotate lines in terminal.\n");
    }

    terminal->cursor.x = 0;

    ret = terminal_empty_line(terminal, terminal->cursor.y);
    ASSERT(ret == 0, "failed to empty new line.\n");

    return 0;

fail:
    return -1;
}

int terminal_push(Terminal* terminal, char* buf, int len){
    int ret;
    int i;
    for (i = 0; i < len; i++){
        char curr = buf[i] & 0xFF;
        if (curr == '\r'){
            char next = buf[i + 1] & 0xFF;
            if (next == '\n'){
                ret = terminal_new_line(terminal);
                ASSERT(ret == 0, "failed to make terminal new line.\n");

                i++;
            }
        }else{
            ELEMENT.character_code = buf[i] & 0xFF;
            ret = terminal_forward_cursor(terminal);
        }

        ASSERT(ret == 0, "failed to move cursor forward.\n");
    }
    return 0;

fail:
    return -1;
}

Element* terminal_element(Terminal* terminal, int x, int y){
    Element* element = NULL;

    element = &terminal->screen[(((y + terminal->start_line_index) % terminal->rows_number) * terminal->cols_number) + x];

    return element;
}
