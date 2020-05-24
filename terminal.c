#include "terminal.h"
#include "common.h"
#include "utf8.h"
#include "color.h"

#include <stdlib.h>
#include <string.h>


#define ELEMENT_X (terminal->cursor.x)
#define ELEMENT_Y ((terminal->start_line_index + terminal->cursor.y) % terminal->rows_number)
#define ELEMENT (terminal->screen[(ELEMENT_Y * terminal->cols_number) + ELEMENT_X])

// modes definitions
#define ESC_MODE   (1 << 0)
#define CSI_MODE   (1 << 1)

// mode operations
#define IS_MODE(x)       (terminal->mode & x)
#define SET_MODE(x)      (terminal->mode |= x)
#define SET_NO_MODE(x)   (terminal->mode &= (~x))

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
    int real_y = ((terminal->start_line_index + y) % terminal->rows_number);
    memset( &terminal->screen[real_y * terminal->cols_number],
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
    LOG("bs handler()\n");
    if (terminal->cursor.x > 0){
        ELEMENT.character_code = 0;
        terminal->cursor.x--;
    }
}

void ht_handler(Terminal* terminal){
    LOG("ht handler()\n");
}

void lf_handler(Terminal* terminal){
    LOG("lf handler()\n");
    terminal_new_line(terminal);
}

void vt_handler(Terminal* terminal){
    LOG("vt handler()\n");
}

void ff_handler(Terminal* terminal){
    LOG("ff handler()\n");
}

void cr_handler(Terminal* terminal){
    LOG("cr handler()\n");
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
    LOG("esc handler()\n");
    SET_MODE(ESC_MODE);
}

void del_handler(Terminal* terminal){
    LOG("del handler()\n");
}

void csi_handler(Terminal* terminal){
    LOG("csi handler()\n");
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
    for (i = 0; i < terminal->csi_parameters_index; i++){
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
    free(parameters);
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

int sgr_set_foreground_color_handler(Terminal* terminal, int* parameters, int left){
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

int sgr_set_background_color_handler(Terminal* terminal, int* parameters, int left){
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

int (*sgr_code_handlers[108])(Terminal* terminal, int* parameters, int left) = {
    [0] = sgr_reset_attributes_handler,
    [1] = sgr_set_bold_handler,
    [4] = sgr_set_underscore_handler,
    [38] = sgr_set_foreground_color_handler,
    [48] = sgr_set_background_color_handler
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
    LOG("csi_cuf_handler()\n");
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
    LOG("csi_cup_handler()\n");
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT((parameters == NULL), "not expecting parameters for this.\n");

    terminal->start_line_index = 0;
    terminal->cursor.x = 0;
    terminal->cursor.y = 0;

    return;

fail:
    csi_free_parameters(parameters);
    return;
}

void csi_ed_handler(Terminal* terminal){
    LOG("csi_ed_handler()\n");
    int ret;
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "failed to get csi parameters.\n");
    ASSERT_TO(fail_on_len, (len > 0), "len of parameters is <= 0.\n");
    
    switch (parameters[0]){
        case (1):
            break;
        case (2):
            ret = terminal_empty(terminal);
            ASSERT_TO(fail_on_len, (ret == 0), "failed to empty terminal\n");
            break;
        case (3):
            break;
        default:
            break;
    }

fail_on_len:
    csi_free_parameters(parameters);
fail:
    return;
}

void csi_el_handler(Terminal* terminal){
    int ret;

    LOG("csi_el_handler()\n");
    // is there parameters?
    if (terminal->csi_parameters_index > 0){

    // default delete from cursor to end of line
    }else{
        int i;
        for (i = terminal->cursor.x; 
             i < terminal->cols_number;
             i++){
            ret = terminal_delete_element(  terminal, 
                                            i,
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete an element.\n");
        }
    }

fail:
    return;
}

void csi_il_handler(Terminal* terminal){
    LOG("csi_il_handler()\n");
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
}

void csi_rm_handler(Terminal* terminal){
    LOG("csi_rm_handler()\n");
}

void csi_sgr_handler(Terminal* terminal){
    LOG("csi_sgr_handler()\n");

    int ret;
    int i;
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "failed to get csi parameters.\n");
    ASSERT_TO(fail_on_len, (len > 0), "len of parameters is <= 0.\n");

    csi_log_parameters(parameters, len);

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


fail_on_len:
    csi_free_parameters(parameters);
fail:
    return;
}

void csi_dsr_handler(Terminal* terminal){
    LOG("csi_dsr_handler()\n");
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

    if (IS_MODE(CSI_MODE)){
        // check if parameter.
        if ((control_code >= '0' && control_code <= '9') || // is number?
            (control_code == ';')){                         // is semicolon

            // too much parameters.
            if (terminal->csi_parameters_index >= CSI_MAX_PARAMETERS_CHARS){
                memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
                terminal->csi_parameters_index = 0;

                SET_NO_MODE(CSI_MODE);
                SET_NO_MODE(ESC_MODE);

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

        memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
        terminal->csi_parameters_index = 0;

        SET_NO_MODE(CSI_MODE);
        SET_NO_MODE(ESC_MODE);

        return FALSE;
    }

    if (IS_MODE(ESC_MODE)){
        if (esc_code_handlers[control_code]){
            (esc_code_handlers[control_code])(terminal);
            return TRUE;
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

    element = &terminal->screen[(((y + terminal->start_line_index) % terminal->rows_number) * terminal->cols_number) + x];

    return element;
}

int terminal_delete_element(Terminal* terminal, int x, int y){
    Element* element = NULL;

    element = &terminal->screen[(((y + terminal->start_line_index) % terminal->rows_number) * terminal->cols_number) + x];

    element->character_code = 0;

    return 0;
}

