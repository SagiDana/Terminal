#include "terminal.h"
#include "common.h"
#include "utf8.h"

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

    terminal->csi_parameters_index = 0;

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
}

void csi_ed_handler(Terminal* terminal){
    LOG("csi_ed_handler()\n");
}

void csi_el_handler(Terminal* terminal){
    LOG("csi_el_handler()\n");
    // is there parameters?
    if (terminal->csi_parameters_index > 0){

    }else{
    }
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
            if (terminal->csi_parameters_index >= sizeof(terminal->csi_parameters)){
                memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
                terminal->csi_parameters_index = 0;

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

            return TRUE;
        }

        memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
        terminal->csi_parameters_index = 0;

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

