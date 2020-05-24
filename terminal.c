#include "terminal.h"
#include "common.h"
#include "utf8.h"
#include "color.h"

#include <stdlib.h>
#include <string.h>


#define ELEMENT_X (terminal->cursor.x)
#define ELEMENT_Y ((terminal->start_line_index + terminal->cursor.y) % terminal->rows_number)
#define ELEMENT (terminal->screen[(ELEMENT_Y * terminal->cols_number) + ELEMENT_X])
#define REAL_Y(y) ((terminal->start_line_index + y) % terminal->rows_number)

// modes definitions
#define ESC_MODE        (1 << 0)
#define CSI_MODE        (1 << 1)
#define OSC_MODE        (1 << 2)
#define PRIVATE_MODE    (1 << 3)

// mode operations
#define IS_MODE(x)       (terminal->mode & x)
#define SET_MODE(x)      (terminal->mode |= x)
#define SET_NO_MODE(x)   (terminal->mode &= (~x))

// mode operations
#define RESET_ATTR()     (terminal->attributes &= 0)
#define IS_ATTR(x)       (terminal->attributes & x)
#define SET_ATTR(x)      (terminal->attributes |= x)
#define SET_NO_ATTR(x)   (terminal->attributes &= (~x))


Terminal* terminal_create(  TPty* pty,
                            int cols_number, 
                            int rows_number, 
                            char* background_color,
                            char* foreground_color){
    int ret;
    Terminal* terminal = NULL;

    terminal = (Terminal*) malloc(sizeof(Terminal));
    ASSERT(terminal, "failed to malloc() terminal.\n");
    memset(terminal, 0, sizeof(Terminal));

    terminal->pty = pty;

    terminal->cols_number = cols_number;
    terminal->rows_number = rows_number;

    terminal->start_line_index = 0;

    // start position.
    terminal->cursor.x = 0;
    terminal->cursor.y = 0;

    ret = color_from_string(background_color, &terminal->default_background_color);
    ASSERT_TO(fail_on_screen, (ret == 0), "failed to convert color string.\n");
    ret = color_from_string(foreground_color, &terminal->default_foreground_color);
    ASSERT_TO(fail_on_screen, (ret == 0), "failed to convert color string.\n");

    terminal->foreground_color = terminal->default_foreground_color;
    terminal->background_color = terminal->default_background_color;

    terminal->csi_parameters_index = 0;
    terminal->attributes = 0;

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
    int ret;

    if (terminal->cursor.x < terminal->cols_number){
        terminal->cursor.x++;
        return 0;
    }

    ret = terminal_new_line(terminal);
    ASSERT(ret == 0, "failed to new line terminal.\n");
    terminal->cursor.x = 0;

    return 0;
    
fail:
    return -1;
}

int terminal_empty_line(Terminal* terminal, int y){
    memset( &terminal->screen[REAL_Y(y) * terminal->cols_number],
            0,
            sizeof(Element) * terminal->cols_number);

    return 0;
}

int terminal_empty(Terminal* terminal){
    int ret;
    int y;

    for (y = 0; y < terminal->rows_number; y++){
        ret = terminal_empty_line(terminal, y);
        ASSERT(ret == 0, "failed to empty line\n");
    }
    return 0;

fail:
    return -1;
}

int terminal_new_line(Terminal* terminal){
    int ret;

    if (terminal->cursor.y + 1 < terminal->rows_number){
        terminal->cursor.y++;
    }else{
        ret = terminal_rotate_lines(terminal);
        ASSERT(ret == 0, "failed to rotate lines in terminal.\n");
    }

    ret = terminal_empty_line(terminal, terminal->cursor.y);
    ASSERT(ret == 0, "failed to empty new line.\n");

    return 0;

fail:
    return -1;
}

int terminal_move_line(Terminal* terminal, int src_y, int dst_y){

    ASSERT((BETWEEN(src_y, 0, terminal->rows_number)), 
            "src_y is not in range.\n");

    ASSERT((BETWEEN(dst_y, 0, terminal->rows_number)), 
            "dst_y is not in range.\n");

    memcpy(&terminal->screen[REAL_Y(dst_y) * terminal->cols_number],
           &terminal->screen[REAL_Y(src_y) * terminal->cols_number],
           (sizeof(Element) * terminal->cols_number));

    return 0;
fail:
    return -1;
}

int terminal_scrolldown(Terminal* terminal, int y, int lines_number){
    int ret;
    int i;

    ASSERT((lines_number > 0), "lines number to scroll invalid.\n");

    int left_lines = (terminal->rows_number - 1) - y - lines_number;

    // we might not care about it 
    // and can just empty all lines
    ASSERT((left_lines >= 0), "too much to scroll\n");

    // the order is important here so we dont 
    // override the lines we need to copy.
    // so we move the lines from the end to the start.
    for (i = 0; i < left_lines; i++){
        ret = terminal_move_line(   terminal,
                (y + left_lines) - i,
                (terminal->rows_number - 1) - i);
        ASSERT(ret == 0, "failed to move line.\n");
    }

    int empty_lines = (terminal->rows_number - 1) - y - left_lines;

    for (i = 0; i < empty_lines; i++){
        ret = terminal_empty_line(terminal, y + i);
        ASSERT(ret == 0, "failed to empty line.\n");
    }

    return 0;
fail:
    return -1;
}

// ----------------------------------------------------------------------
// Control code handlers
// Everything here is directly based on this specs:
// http://man7.org/linux/man-pages/man4/console_codes.4.html
// ----------------------------------------------------------------------

void null_handler(Terminal* terminal){
    LOG("NULL control!\n");
}

void bel_handler(Terminal* terminal){
    // I hate this thing, might not even implement this shit!
    LOG("bel handler()\n");
}

void bs_handler(Terminal* terminal){
    if (terminal->cursor.x > 0){
        ELEMENT.character_code = 0;
        terminal->cursor.x--;
    }
}

void ht_handler(Terminal* terminal){
    LOG("ht handler()\n");
}

void lf_handler(Terminal* terminal){
    terminal_new_line(terminal);
}

void vt_handler(Terminal* terminal){
    LOG("vt handler()\n");
}

void ff_handler(Terminal* terminal){
    LOG("ff handler()\n");
}

void cr_handler(Terminal* terminal){
    terminal->cursor.x = 0;
}

void so_handler(Terminal* terminal){
    LOG("so handler()\n");
}

void si_handler(Terminal* terminal){
    LOG("si handler()\n");
}

void can_handler(Terminal* terminal){
    LOG("can handler()\n");
    SET_NO_MODE(ESC_MODE);
}

void sub_handler(Terminal* terminal){
    LOG("sub handler()\n");
    SET_NO_MODE(ESC_MODE);
}

void esc_handler(Terminal* terminal){
    SET_MODE(ESC_MODE);
}

void del_handler(Terminal* terminal){
    LOG("del handler()\n");
}

void csi_handler(Terminal* terminal){
    SET_MODE(CSI_MODE);
}

// 0x100 the maximum value of control code?
void (*control_code_handlers[0x100])(Terminal* terminal) = {
    [0x00] = null_handler,
    [0x07] = bel_handler,
    [0x08] = bs_handler,
    [0x09] = ht_handler,
    [0x0A] = lf_handler,
    [0x0B] = vt_handler,
    [0x0C] = ff_handler,
    [0x0D] = cr_handler,
    [0x0E] = so_handler,
    [0x0F] = si_handler,
    [0x18] = can_handler,
    [0x1A] = sub_handler,
    [0x1B] = esc_handler,
    [0x7F] = del_handler,
    [0x9B] = csi_handler
};

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Esc code handlers
// ----------------------------------------------------------------------

void esc_ris_handler(Terminal* terminal){
    LOG("esc_ris_handler()\n");
}

void esc_ind_handler(Terminal* terminal){
    LOG("esc_ind_handler()\n");
}

void esc_nel_handler(Terminal* terminal){
    LOG("esc_nel_handler()\n");
}

void esc_hts_handler(Terminal* terminal){
    LOG("esc_hts_handler()\n");
}

void esc_ri_handler(Terminal* terminal){
    LOG("esc_ri_handler()\n");
}

void esc_decid_handler(Terminal* terminal){
    LOG("esc_decid_handler()\n");
}

void esc_decsc_handler(Terminal* terminal){
    LOG("esc_decsc_handler()\n");
}

void esc_decrc_handler(Terminal* terminal){
    LOG("esc_decrc_handler()\n");
}

void esc_select_charset_handler(Terminal* terminal){
    LOG("esc_select_charset_handler()\n");
}

void esc_define_g0_charset_handler(Terminal* terminal){
    LOG("esc_define_g0_charset_handler()\n");
}

void esc_define_g1_charset_handler(Terminal* terminal){
    LOG("esc_define_g1_charset_handler()\n");
}

void esc_decpnm_handler(Terminal* terminal){
    LOG("esc_decpnm_handler()\n");
}

void esc_decpam_handler(Terminal* terminal){
    LOG("esc_decpam_handler()\n");
}

void esc_osc_handler(Terminal* terminal){
    LOG("esc_osc_handler()\n");
    SET_MODE(OSC_MODE);
}

void (*esc_code_handlers[100])(Terminal* terminal) = {
    ['c'] = esc_ris_handler,
    ['D'] = esc_ind_handler,
    ['E'] = esc_nel_handler,
    ['H'] = esc_hts_handler,
    ['M'] = esc_ri_handler,
    ['Z'] = esc_decid_handler,
    ['7'] = esc_decsc_handler,
    ['8'] = esc_decrc_handler,
    ['['] = csi_handler,
    ['%'] = esc_select_charset_handler,
    ['('] = esc_define_g0_charset_handler,
    [')'] = esc_define_g1_charset_handler,
    ['>'] = esc_decpnm_handler,
    ['='] = esc_decpam_handler,
    [']'] = esc_osc_handler
};

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------

int* csi_get_parameters(Terminal* terminal, int* len){
    int* parameters = NULL;
    int i;

    ASSERT((terminal->csi_parameters_index > 0), "no csi parameters.\n");

    // 16 is the maximum csi parameters.
    parameters = (int*) malloc(sizeof(int) * CSI_MAX_PARAMETERS_CHARS);
    memset(parameters, 0, sizeof(int) * CSI_MAX_PARAMETERS_CHARS);

    *len = 0;
    int parameter_start_index = 0;

    i = 0;
    if (terminal->csi_parameters[0] == '?'){
        SET_MODE(PRIVATE_MODE);
        i = 1; // skip question mark
    }
    for (; i < terminal->csi_parameters_index; i++){
        if (terminal->csi_parameters[i] == ';'){
            // null terminating the current parameter.
            terminal->csi_parameters[i] = 0;
            // convert parameter to int.
            parameters[(*len)] = atoi((char*) &terminal->csi_parameters[parameter_start_index]);

            // setting paramete_start_index to next parameter
            parameter_start_index = i + 1;
            
            (*len)++;
        }
    }

    // null terminating the current parameter.
    terminal->csi_parameters[i] = 0;
    // convert parameter to int.
    parameters[(*len)] = atoi((char*) &terminal->csi_parameters[parameter_start_index]);

    // setting paramete_start_index to next parameter
    parameter_start_index = i + 1;

    (*len)++;

    // reset terminal's csi_parameters (they are not valid any more).
    memset(terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
    terminal->csi_parameters_index = 0;

    return parameters;

fail:
    return NULL;
}

void csi_free_parameters(int* parameters){
    if (parameters){
        free(parameters);
    }
}

void csi_log_parameters(int* parameters, int len){
    int i;
    LOG("csi parameters: ");
    for (i = 0; i < len; i++){
        LOG("%d;", parameters[i]);
    }
    LOG("\n");
}

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// CSI->SGR code handlers
// ----------------------------------------------------------------------

int sgr_reset_attributes_handler(Terminal* terminal, int* parameters, int left){
    RESET_ATTR();
    terminal->background_color = terminal->default_background_color;
    terminal->foreground_color = terminal->default_foreground_color;

    // this handler does not read more parameters.
    return 0;
}

int sgr_set_bold_handler(Terminal* terminal, int* parameters, int left){
    SET_ATTR(BOLD_ATTR);

    // this handler does not read more parameters.
    return 0;
}

int sgr_set_underscore_handler(Terminal* terminal, int* parameters, int left){
    SET_ATTR(UNDERSCORE_ATTR);

    // this handler does not read more parameters.
    return 0;
}

int sgr_set_foreground_24bit_color_handler(Terminal* terminal, int* parameters, int left){
    ASSERT((left >= 2), "not enough parameters left.\n");

    // means 24bit color: r.g.b colors in next parameters
    if (parameters[1] == 2){
        ASSERT((left >= 5), "not enough parameters left.\n");

        terminal->foreground_color = TRUE_COLOR_COLOR(  parameters[2],
                                                        parameters[3], 
                                                        parameters[4]);
        return 4;
    }

    // means 256 color
    if (parameters[1] == 5){
        ASSERT((left >= 3), "not enough parameters left.\n");

        unsigned int color = parameters[2];
        LOG("256 color: %d\n", color);

        return 2;
    }

fail:
    return -1;
}

int sgr_set_background_24bit_color_handler(Terminal* terminal, int* parameters, int left){
    ASSERT((left >= 2), "not enough parameters left.\n");

    // means 24bit color: r.g.b colors in next parameters
    if (parameters[1] == 2){
        ASSERT((left >= 5), "not enough parameters left.\n");

        terminal->background_color = TRUE_COLOR_COLOR(  parameters[2],
                                                        parameters[3], 
                                                        parameters[4]);
        return 4;
    }

    // means 256 color
    if (parameters[1] == 5){
        ASSERT((left >= 3), "not enough parameters left.\n");

        unsigned int color = parameters[2];
        LOG("256 color: %d\n", color);

        return 2;
    }

fail:
    return -1;
}

int sgr_set_foreground_color_handler(Terminal* terminal, int* parameters, int left){
    return 0;
}

int sgr_set_background_color_handler(Terminal* terminal, int* parameters, int left){
    LOG("sgr_set_background_color_handler()\n");
    return 0;
}

int sgr_reverse_video_off_handler(Terminal* terminal, int* parameters, int left){
    LOG("sgr_reverse_video_off_handler()\n");
    return 0;
}

int (*sgr_code_handlers[108])(Terminal* terminal, int* parameters, int left) = {
    [0] = sgr_reset_attributes_handler,
    [1] = sgr_set_bold_handler,
    [4] = sgr_set_underscore_handler,

    [27] = sgr_reverse_video_off_handler,

    [30] = sgr_set_foreground_color_handler,
    [31] = sgr_set_foreground_color_handler,
    [32] = sgr_set_foreground_color_handler,
    [33] = sgr_set_foreground_color_handler,
    [34] = sgr_set_foreground_color_handler,
    [35] = sgr_set_foreground_color_handler,
    [36] = sgr_set_foreground_color_handler,
    [37] = sgr_set_foreground_color_handler,
    [38] = sgr_set_foreground_24bit_color_handler,
    [39] = sgr_set_foreground_color_handler, // default

    [40] = sgr_set_background_color_handler,
    [41] = sgr_set_background_color_handler,
    [42] = sgr_set_background_color_handler,
    [43] = sgr_set_background_color_handler,
    [44] = sgr_set_background_color_handler,
    [45] = sgr_set_background_color_handler,
    [46] = sgr_set_background_color_handler,
    [47] = sgr_set_background_color_handler,
    [48] = sgr_set_background_24bit_color_handler,
    [49] = sgr_set_background_color_handler, // default

    [90] = sgr_set_foreground_color_handler,
    [91] = sgr_set_foreground_color_handler,
    [92] = sgr_set_foreground_color_handler,
    [93] = sgr_set_foreground_color_handler,
    [94] = sgr_set_foreground_color_handler,
    [95] = sgr_set_foreground_color_handler,
    [96] = sgr_set_foreground_color_handler,
    [97] = sgr_set_foreground_color_handler,
    [100] = sgr_set_background_color_handler,
    [101] = sgr_set_background_color_handler,
    [102] = sgr_set_background_color_handler,
    [103] = sgr_set_background_color_handler,
    [104] = sgr_set_background_color_handler,
    [105] = sgr_set_background_color_handler,
    [106] = sgr_set_background_color_handler,
    [107] = sgr_set_background_color_handler
};

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// CSI code handlers
// ----------------------------------------------------------------------

void csi_ich_handler(Terminal* terminal){
    LOG("csi_ich_handler()\n");
}

void csi_cuu_handler(Terminal* terminal){
    LOG("csi_cuu_handler()\n");
}

void csi_cud_handler(Terminal* terminal){
    LOG("csi_cud_handler()\n");
}

void csi_cuf_handler(Terminal* terminal){
    int rows;
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "no parameters - no where to move.\n");

    ASSERT_TO(fail_on_parameters, (len == 1), "num of parameters is not 1.\n");

    rows = parameters[0];
    ASSERT_TO(fail_on_parameters, 
                (BETWEEN(rows, 0, terminal->cols_number - terminal->cursor.x)), 
                "num of columns is not in range.\n");

    terminal->cursor.x += rows;

fail_on_parameters:
    csi_free_parameters(parameters);
fail:
    return;
}

void csi_cub_handler(Terminal* terminal){
    LOG("csi_cub_handler()\n");
}

void csi_cnl_handler(Terminal* terminal){
    LOG("csi_cnl_handler()\n");
}

void csi_cpl_handler(Terminal* terminal){
    LOG("csi_cpl_handler()\n");
}

void csi_cha_handler(Terminal* terminal){
    LOG("csi_cha_handler()\n");
}

void csi_cup_handler(Terminal* terminal){
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL){
        // terminal->start_line_index = 0;
        terminal->cursor.x = 0;
        terminal->cursor.y = 0;
        return;
    }

    ASSERT((len == 2), "num of parameters is not equal to 2.\n");

    int row = parameters[0];
    int col = parameters[1];

    ASSERT((BETWEEN(row, 0, terminal->rows_number)), "row number is not valid.\n");
    ASSERT((BETWEEN(col, 0, terminal->cols_number)), "col number is not valid.\n");

    terminal->cursor.x = col;
    terminal->cursor.y = row;

fail:
    csi_free_parameters(parameters);
    return;
}

void csi_ed_handler(Terminal* terminal){
    int ret;
    int len = 0;
    int* parameters = NULL;
    parameters = csi_get_parameters(terminal, &len);
    
    // default: from cursor to end of display
    if (parameters == NULL){ // default.
        int i;
        for (i = terminal->cursor.x; i < terminal->cols_number; i++){
            ret = terminal_delete_element(  terminal, 
                                            i, 
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete element.\n");
        }
        for (i = terminal->cursor.y; i < terminal->rows_number; i++){
            ret = terminal_empty_line(terminal, i);
            ASSERT(ret == 0, "failed to empty line.\n");
        }
        return;
    }
    
    ASSERT((len == 1), "number of parameters is not 1.\n");

    if (parameters[0] == 1){ // from start to cursor
        int i;
        for (i = 0; i < terminal->cursor.y; i++){
            ret = terminal_empty_line(terminal, i);
            ASSERT(ret == 0, "failed to empty line.\n");
        }
        for (i = 0; i <= terminal->cursor.x; i++){
            ret = terminal_delete_element(  terminal, 
                                            i, 
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete element.\n");
        }
    } 
    if (parameters[0] == 2){ // all display
        ret = terminal_empty(terminal);
        ASSERT(ret == 0, "failed to empty terminal.\n");
    }
    if (parameters[0] == 3){ // all display + scrollback
        ret = terminal_empty(terminal);
        ASSERT(ret == 0, "failed to empty terminal.\n");
        // TODO erase scrollback when we have one.
    }

fail:
    csi_free_parameters(parameters);
    return;
}

void csi_el_handler(Terminal* terminal){
    int ret;
    int len = 0;
    int* parameters = NULL;
    parameters = csi_get_parameters(terminal, &len);

    // default: from cursor to end of line 
    if (parameters == NULL){ // default.
        int i;
        for (i = terminal->cursor.x; i < terminal->cols_number; i++){
            ret = terminal_delete_element(  terminal, 
                                            i, 
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete element.\n");
        }
        return;
    }
    
    ASSERT((len == 1), "number of parameters is not 1.\n");

    if (parameters[0] == 1){ // from start to cursor
        int i;
        for (i = 0; i <= terminal->cursor.x; i++){
            ret = terminal_delete_element(  terminal, 
                                            i, 
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete element.\n");
        }
    } 
    if (parameters[0] == 2){ // all line
        ret = terminal_empty_line(terminal, terminal->cursor.y);
        ASSERT(ret == 0, "failed to empty line.\n");
    }

fail:
    csi_free_parameters(parameters);
    return;
}

void csi_il_handler(Terminal* terminal){
    LOG("csi_il_handler()\n");
    int ret;
    int len = 0;
    int* parameters = NULL;
    int blank_lines_number;

    parameters = csi_get_parameters(terminal, &len);

    if (parameters == NULL){
        blank_lines_number = 1;
    }

    ASSERT((len <= 1), "too many parameters.\n");

    // ASSERT( BETWEEN(blank_lines_number, terminal->cursor.y, terminal->rows_number), 
            // "blank lines is more than screen has.\n");

    ret = terminal_scrolldown(  terminal, 
                                terminal->cursor.y, 
                                blank_lines_number);
    ASSERT(ret == 0, "failed to scroll down.\n");

fail:
    csi_free_parameters(parameters);
}

void csi_dl_handler(Terminal* terminal){
    LOG("csi_dl_handler()\n");
}

void csi_dch_handler(Terminal* terminal){
    LOG("csi_dch_handler()\n");
}

void csi_ech_handler(Terminal* terminal){
    LOG("csi_ech_handler()\n");
}

void csi_hpr_handler(Terminal* terminal){
    LOG("csi_hpr_handler()\n");
}

void csi_da_handler(Terminal* terminal){
    LOG("csi_da_handler()\n");
}

void csi_vpa_handler(Terminal* terminal){
    LOG("csi_vpa_handler()\n");
}

void csi_vpr_handler(Terminal* terminal){
    LOG("csi_vpr_handler()\n");
}

void csi_hvp_handler(Terminal* terminal){
    LOG("csi_hvp_handler()\n");
}

void csi_tbc_handler(Terminal* terminal){
    LOG("csi_tbc_handler()\n");
}

void csi_sm_handler(Terminal* terminal){
    LOG("csi_sm_handler()\n");
    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "failed to get csi parameters.\n");

    csi_log_parameters(parameters, len);

    SET_NO_MODE(PRIVATE_MODE);

    csi_free_parameters(parameters);
fail:
    return;
}

void csi_rm_handler(Terminal* terminal){
    LOG("csi_rm_handler()\n");
    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "failed to get csi parameters.\n");

    csi_log_parameters(parameters, len);

    SET_NO_MODE(PRIVATE_MODE);

    csi_free_parameters(parameters);
fail:
    return;
}

void csi_sgr_handler(Terminal* terminal){
    int ret;
    int i;
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL){
        sgr_reset_attributes_handler(terminal, NULL, 0);
        return;
    }

    for (i = 0; i < len; i++){
        int left = len - i;

        // to prevent out of bound.
        ASSERT(((parameters[i] >= 0) && (parameters[i] < 200)), 
               "sgr parameter is not in range.\n");

        ASSERT(sgr_code_handlers[parameters[i]], 
               "handler to sgr parameter does not exist.\n");

        ret = (sgr_code_handlers[parameters[i]])(terminal,
                                                 &parameters[i],
                                                 left);
        ASSERT((ret >= 0), "handler failed for parameter: %d\n",
                            parameters[i]);

        i += ret; // continue to next parameters.
    }

    csi_free_parameters(parameters);

fail:
    return;
}

void csi_dsr_handler(Terminal* terminal){
    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "no csi parameters.\n");

    // report cursor position.
    if (parameters[0] == 6){
        char buf[100];
        int buf_len;
        int ret;
        buf_len = sprintf(  buf, 
                            "\033[%d;%dR", 
                            terminal->cursor.y + 1, 
                            terminal->cursor.x + 1);

        ret = pty_write(terminal->pty, 
                        buf,
                        buf_len);
        ASSERT_TO(fail_on_pty_write, (ret >= 0), "failed to write to pty.\n");
    }

fail_on_pty_write:
    csi_free_parameters(parameters);
fail:
    return;
}

void csi_decll_handler(Terminal* terminal){
    LOG("csi_decll_handler()\n");
}

void csi_decstbm_handler(Terminal* terminal){
    LOG("csi_decstbm_handler()\n");
}

void csi_save_cursor_handler(Terminal* terminal){
    LOG("csi_save_cursor_handler()\n");
}

void csi_restore_cursor_handler(Terminal* terminal){
    LOG("csi_restore_cursor_handler()\n");
}

void csi_hpa_handler(Terminal* terminal){
    LOG("csi_hpa_handler()\n");
}

void (*csi_code_handlers[200])(Terminal* terminal) = {
    ['@'] = csi_ich_handler,
    ['A'] = csi_cuu_handler,
    ['B'] = csi_cud_handler,
    ['C'] = csi_cuf_handler,
    ['D'] = csi_cub_handler,
    ['E'] = csi_cnl_handler,
    ['F'] = csi_cpl_handler,
    ['G'] = csi_cha_handler,
    ['H'] = csi_cup_handler,
    ['J'] = csi_ed_handler,
    ['K'] = csi_el_handler,
    ['L'] = csi_il_handler,
    ['M'] = csi_dl_handler,
    ['P'] = csi_dch_handler,
    ['X'] = csi_ech_handler,
    ['a'] = csi_hpr_handler,
    ['c'] = csi_da_handler,
    ['d'] = csi_vpa_handler,
    ['e'] = csi_vpr_handler,
    ['f'] = csi_hvp_handler,
    ['g'] = csi_tbc_handler,
    ['h'] = csi_sm_handler,
    ['l'] = csi_rm_handler,
    ['m'] = csi_sgr_handler,
    ['n'] = csi_dsr_handler,
    ['q'] = csi_decll_handler,
    ['r'] = csi_decstbm_handler,
    ['s'] = csi_save_cursor_handler,
    ['u'] = csi_restore_cursor_handler,
    ['`'] = csi_hpa_handler
};

// ----------------------------------------------------------------------

int handle_control_codes(Terminal* terminal, unsigned int character_code){
    unsigned char control_code = character_code & 0xFF;

	/* 
     * FROM ST:
     * 
	 * STR sequence must be checked before anything else
	 * because it uses all following characters until it
	 * receives a ESC, a SUB, a ST or any other C1 control
	 * character.
	 */
    if (IS_MODE(OSC_MODE)){
        // did end?
		if ((control_code == '\a') || 
            (control_code == 0x18) || 
            (control_code == 0x1A) || 
            (control_code == 0x1B) ||
		    (BETWEEN(control_code, 0x80, 0x9F))) {
            SET_NO_MODE(OSC_MODE);
        }

        return TRUE;
    }

    if (IS_MODE(CSI_MODE)){
        // check if parameter.
        if ((control_code >= '0' && control_code <= '9') || // is number?
            (control_code == ';') ||                        // is semicolon
            (control_code == '?')){                         // is question mark (for the beginning)

            // too much parameters.
            if (terminal->csi_parameters_index >= CSI_MAX_PARAMETERS_CHARS){
                memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
                terminal->csi_parameters_index = 0;

                SET_NO_MODE(CSI_MODE);
                SET_NO_MODE(ESC_MODE);

                LOG("too much parameters\n");

                return FALSE;
            }

            terminal->csi_parameters[terminal->csi_parameters_index] = control_code;
            terminal->csi_parameters_index++;

            return TRUE;
        }

        if (csi_code_handlers[control_code]){
            (csi_code_handlers[control_code])(terminal);

            memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
            terminal->csi_parameters_index = 0;

            SET_NO_MODE(CSI_MODE);
            SET_NO_MODE(ESC_MODE);

            return TRUE;
        }

        LOG("no csi handler found for: %d.\n", control_code);
        LOG("parameters for debuging: \"%s\".\n", terminal->csi_parameters);

        memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
        terminal->csi_parameters_index = 0;

        SET_NO_MODE(CSI_MODE);
        SET_NO_MODE(ESC_MODE);

        return FALSE;
    }

    if (IS_MODE(ESC_MODE)){
        if (esc_code_handlers[control_code]){
            (esc_code_handlers[control_code])(terminal);

            SET_NO_MODE(ESC_MODE);
            return TRUE;
        }else{
            LOG("no esc handler found for: %u\n", control_code);
        }
    }

    if (control_code_handlers[control_code]){
        (control_code_handlers[control_code])(terminal);
        return TRUE;
    }

    return FALSE;
}

/*
 * This it the main function of the terminal. it gets 
 * character_code as input and handle control code if they appear.
 */

int terminal_emulate(Terminal* terminal, unsigned int character_code){
    int ret;

    ret = handle_control_codes(terminal, character_code);
    if (ret == TRUE){
        return 0;
    }

    // not a control code:
    // insert simple element to the terminal and moving
    // cursor forward.
    ELEMENT.character_code = character_code;
    ELEMENT.foreground_color = terminal->foreground_color;
    ELEMENT.background_color = terminal->background_color;
    ELEMENT.attributes = terminal->attributes;

    ret = terminal_forward_cursor(terminal);
    ASSERT(ret == 0, "failed to move cursor forward.\n");

    return 0;

fail:
    return -1;
}

int terminal_push(Terminal* terminal, char* buf, int len){
    int ret;
    int i;

    unsigned int utf8_state = UTF8_ACCEPT;
    unsigned int utf8_codepoint;

    for (i = 0; i < len; i++){
        char curr = buf[i];

        // unicode 
        if (!utf8_decode(&utf8_state, 
                         &utf8_codepoint, 
                         (unsigned char) curr)){

            ret = terminal_emulate(terminal, utf8_codepoint);
            ASSERT(ret == 0, "failed in emulate.\n");

            utf8_codepoint = 0;
        }
        
        if (utf8_state == UTF8_REJECT){
            LOG("utf8 code mallformed\n");

            utf8_state = UTF8_ACCEPT;
            utf8_codepoint = 0;
        }
    }
    return 0;

fail:
    return -1;
}

Element* terminal_element(Terminal* terminal, int x, int y){
    Element* element = NULL;

    element = &terminal->screen[(REAL_Y(y) * terminal->cols_number) + x];

    return element;
}

int terminal_delete_element(Terminal* terminal, int x, int y){
    Element* element = NULL;

    element = &terminal->screen[(REAL_Y(y) * terminal->cols_number) + x];

    memset(element, 0, sizeof(Element));
    return 0;
}

