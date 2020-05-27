#include "terminal.h"
#include "common.h"
#include "utf8.h"
#include "color.h"

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>


#define REAL_Y(y) ((terminal->start_line_index + y) % terminal->rows_number)
#define ELEMENT (terminal->screen[(REAL_Y(terminal->cursor.y) * terminal->cols_number) + terminal->cursor.x])

// VT100 modes
#define VT_LMN_MODE         (1 << 0) // New line mode
#define VT_DECCKM_MODE      (1 << 1) // Cursor key to application
#define VT_DECANM_MODE      (1 << 2) // ANSI
#define VT_DECCOLM_MODE     (1 << 3) // Number of columns to 132
#define VT_DECSCLM_MODE     (1 << 4) // Smooth scrolling
#define VT_DECSCNM_MODE     (1 << 5) // Reverse video
#define VT_DECOM_MODE       (1 << 6) // Origin to relative
#define VT_DECAWM_MODE      (1 << 7) // Auto-wrap mode
#define VT_DECARM_MODE      (1 << 8) // Auto-repeate mode
#define VT_DECINLM_MODE     (1 << 9) // Interlacing mode
#define VT_DECKPAM_MODE     (1 << 10) // alternative/numeric keypad mode

// mode operations
#define IS_VT_MODE(x)       (terminal->vt_mode & x)
#define SET_VT_MODE(x)      (terminal->vt_mode |= x)
#define SET_NO_VT_MODE(x)   (terminal->vt_mode &= (~x))

// terminal state machine modes definitions
#define ESC_MODE        (1 << 0)
#define G0_MODE         (1 << 1) // define G0 charset.
#define G1_MODE         (1 << 2) // define G1 charset.
#define CSI_MODE        (1 << 3)
#define OSC_MODE        (1 << 4)
// this mode is currently used by csi_get_parameters() to let us know 
// the '?' character was prefixed the parameters what indicates use we 
// in private mode.
#define PRIVATE_MODE    (1 << 5)
#define CSI_DOL_MODE    (1 << 6)

// mode operations
#define IS_MODE(x)       (terminal->mode & x)
#define SET_MODE(x)      (terminal->mode |= x)
#define SET_NO_MODE(x)   (terminal->mode &= (~x))

// mode operations
#define RESET_ATTR()     (terminal->attributes &= 0)
#define IS_ATTR(x)       (terminal->attributes & x)
#define SET_ATTR(x)      (terminal->attributes |= x)
#define SET_NO_ATTR(x)   (terminal->attributes &= (~x))


// ------------------------------------------------------------
// debug escaped handlers macros.
// ------------------------------------------------------------

#define ESC_DEBUG
#define CSI_DEBUG
#define SGR_DEBUG

// #undef ESC_DEBUG
// #undef CSI_DEBUG
// #undef SGR_DEBUG

#ifdef ESC_DEBUG
#define DEBUG_ESC_HANDLER(handler) do {                                 \
    LOG("DEBUG_ESC: handler: %s.\n", handler);                          \
}while(0)
#else
#define DEBUG_ESC_HANDLER(handler) do{                                  \
}while(0)
#endif

#ifdef CSI_DEBUG
#define DEBUG_CSI_HANDLER(handler) do {                                 \
    LOG("DEBUG_CSI: handler: %s.\n", handler);                          \
    LOG("DEBUG_CSI: parameters:\"%s\".\n", terminal->csi_parameters);   \
}while(0)
#else
#define DEBUG_CSI_HANDLER(handler) do{                                  \
}while(0)
#endif

#ifdef SGR_DEBUG
#define DEBUG_SGR_HANDLER(handler) do {                                 \
    LOG("DEBUG_SGR: handler: %s.\n", handler);                          \
    LOG("DEBUG_SGR: parameters:\n");                                    \
    for (int i = 0; i < left; i++){                                     \
        LOG("\t parameter[%d]: %d\n", i, parameters[i]);                \
    }                                                                   \
}while(0)
#else
#define DEBUG_SGR_HANDLER(handler) do{                                  \
}while(0)
#endif

// ------------------------------------------------------------



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

    terminal->top = 0;
    terminal->bottom = rows_number - 1;

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

    terminal->osc_buffer_index = 0;

    terminal->csi_parameters_index = 0;
    terminal->attributes = 0;

    terminal->screen = (TElement*) malloc(sizeof(TElement) * cols_number * rows_number);
    ASSERT_TO(fail_on_screen, terminal->screen, "failed to malloc screen.\n");
    memset(terminal->screen, 0, sizeof(TElement) * cols_number * rows_number);

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
    ASSERT(terminal->screen, "trying to resize without any screen.\n");

    free(terminal->screen);
    terminal->screen = NULL;

    terminal->cols_number = cols_number;
    terminal->rows_number = rows_number;

    terminal->top = 0;
    terminal->bottom = rows_number - 1;

    terminal->cursor.x = 0;
    terminal->cursor.y = 0;
    terminal->start_line_index = 0;

    terminal->screen = (TElement*) malloc(sizeof(TElement) * cols_number * rows_number);
    ASSERT(terminal->screen, "failed to malloc screen.\n");
    memset(terminal->screen, 0, sizeof(TElement) * cols_number * rows_number);

    // Notify shell change in size!
    pty_resize( terminal->pty, 
                terminal->cols_number,
                terminal->rows_number);
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

    if (terminal->cursor.x + 1 < terminal->cols_number){
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

int terminal_empty_element(Terminal* terminal, int x, int y){
    TElement* element = NULL;

    element = &terminal->screen[(REAL_Y(y) * terminal->cols_number) + x];

    memset(element, 0, sizeof(TElement));
    return 0;
}

int terminal_empty_line(Terminal* terminal, int y){
    memset( &terminal->screen[REAL_Y(y) * terminal->cols_number],
            0,
            sizeof(TElement) * terminal->cols_number);

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
           (sizeof(TElement) * terminal->cols_number));

    return 0;
fail:
    return -1;
}

int terminal_scrollup(Terminal* terminal, int top_y, int bottom_y, int lines_number){
    int ret;
    int i;

    ASSERT((lines_number > 0), "lines number to scroll invalid.\n");
    ASSERT((BETWEEN(top_y, 0, terminal->rows_number - 1)),
           "starting scroll position is not in range.\n");
    ASSERT((BETWEEN(bottom_y, top_y, terminal->rows_number - 1)),
           "starting scroll position is not in range.\n");

    int left_lines = bottom_y - top_y - lines_number;

    // we might not care about it 
    // and can just empty all lines
    ASSERT((left_lines >= 0), "too much to scroll.\n");

    LOG("top_y: %d, bottom_y: %d, lines_number: %d, left_lines: %d\n", 
            top_y,
            bottom_y,
            lines_number,
            left_lines);

    // the order is important here so we dont 
    // override the lines we need to copy.
    // so we move the lines from the end to the start.
    for (i = 0; i < left_lines; i++){
        ret = terminal_move_line(   terminal,
                                    (bottom_y - left_lines) + i,
                                    top_y + i);
        ASSERT(ret == 0, "failed to move line.\n");
    }

    int empty_lines = bottom_y - top_y + left_lines;

    for (i = 0; i < empty_lines; i++){
        ret = terminal_empty_line(terminal, bottom_y - i);
        ASSERT(ret == 0, "failed to empty line.\n");
    }

    return 0;
fail:
    return -1;
}

int terminal_scrolldown(Terminal* terminal, int top_y, int bottom_y, int lines_number){
    int ret;
    int i;

    ASSERT((lines_number > 0), "lines number to scroll invalid.\n");

    ASSERT((BETWEEN(top_y, 0, terminal->rows_number - 1)),
           "starting scroll position is not in range.\n");
    ASSERT((BETWEEN(bottom_y, top_y, terminal->rows_number - 1)),
           "starting scroll position is not in range.\n");

    int left_lines = bottom_y - top_y - lines_number;

    // we might not care about it 
    // and can just empty all lines
    ASSERT((left_lines >= 0), "too much to scroll\n");

    // the order is important here so we dont 
    // override the lines we need to copy.
    // so we move the lines from the end to the start.
    for (i = 0; i < left_lines; i++){
        ret = terminal_move_line(   terminal,
                                    (top_y + left_lines) - i,
                                    bottom_y - i);
        ASSERT(ret == 0, "failed to move line.\n");
    }

    int empty_lines = bottom_y - top_y - left_lines;

    for (i = 0; i < empty_lines; i++){
        ret = terminal_empty_line(terminal, top_y + i);
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
    DEBUG_ESC_HANDLER("null_handler");
}

void bel_handler(Terminal* terminal){
    // I hate this thing, might not even implement this shit!
    DEBUG_ESC_HANDLER("bel_handler");
}

void bs_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("bs_handler");

    if (terminal->cursor.x > 0){
        terminal->cursor.x--;
    }
}

void ht_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("ht_handler");

    int max_x = terminal->cols_number - 1;

    int new_x = terminal->cursor.x + 8;
    new_x &= ~(7); // allign to 8 characters.

    terminal->cursor.x = new_x < max_x ? new_x : max_x;
}

void lf_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("lf_handler");

    terminal_new_line(terminal);
}

void vt_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("vt_handler");
}

void ff_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("ff_handler");
}

void cr_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("cr_handler");

    terminal->cursor.x = 0; // return to start of line.
}

void so_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("so_handler");
}

void si_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("si_handler");
}

void can_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("can_handler");

    SET_NO_MODE(ESC_MODE); // interrupt esc sequence.
}

void sub_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("sub_handler");

    SET_NO_MODE(ESC_MODE); // interrupt esc sequence.
}

void esc_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_handler");

    SET_MODE(ESC_MODE);
}

void del_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("del_handler");
    // ignored?
}

void csi_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("csi_handler");

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
    DEBUG_ESC_HANDLER("esc_ris_handler");
}

void esc_ind_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_ind_handler");

    // TODO: scroll window one line up
}

void esc_nel_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_nel_handler");
    
    // TODO: moe to next line
}

void esc_hts_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_hts_handler");
}

void esc_ri_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_ri_handler");

    // TODO: scroll window one line down
}

void esc_decid_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_decid_handler");
}

void esc_decsc_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_decsc_handler");
}

void esc_decrc_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_decrc_handler");
}

void esc_select_charset_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_select_charset_handler");
}

void esc_define_g0_charset_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_define_g0_charset_handler");

    SET_MODE(G0_MODE);
}

void esc_define_g1_charset_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_define_g1_charset_handler");

    SET_MODE(G1_MODE);
}

void esc_decpnm_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_decpnm_handler");

    SET_NO_VT_MODE(VT_DECKPAM_MODE);
}

void esc_decpam_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_decpam_handler");

    SET_VT_MODE(VT_DECKPAM_MODE);
}

void esc_osc_handler(Terminal* terminal){
    DEBUG_ESC_HANDLER("esc_osc_handler");

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

    if (terminal->csi_parameters_index == 0){ 
        return NULL;
    }

    parameters = (int*) malloc(sizeof(int) * CSI_MAX_PARAMETERS_CHARS);
    memset(parameters, 0, sizeof(int) * CSI_MAX_PARAMETERS_CHARS);

    *len = 0;
    int parameters_start_index = 0;

    i = 0;
    if (terminal->csi_parameters[0] == '?'){
        SET_MODE(PRIVATE_MODE);
        i = 1; // skip question mark
    }

    for (; i < terminal->csi_parameters_index; i++){
        if (terminal->csi_parameters[i] == ';'){
            // null terminating the current parameter.
            terminal->csi_parameters[i] = 0;

            // Convert parameter to int:
            // NOTE: atoi is returning 0 when empty string ("") is given, 
            // which is exactly what we need.
            parameters[(*len)] = atoi((char*) &terminal->csi_parameters[parameters_start_index]);

            // setting paramete_start_index to next parameter
            parameters_start_index = i + 1;
            
            (*len)++;
        }
    }

    // if parameters didn't ended with ';'
    if (parameters_start_index < terminal->csi_parameters_index){
        // null terminating the current parameter.
        terminal->csi_parameters[i] = 0;
        // convert parameter to int.
        parameters[(*len)] = atoi((char*) &terminal->csi_parameters[parameters_start_index]);

        (*len)++;
    }

    // reset terminal's csi_parameters (they are not valid any more).
    memset(terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
    terminal->csi_parameters_index = 0;

    if (*len == 0){
        SET_NO_MODE(PRIVATE_MODE);
        free(parameters);
        parameters = NULL;
    }

    return parameters;
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
    DEBUG_SGR_HANDLER("sgr_reset_attributes_handler");

    RESET_ATTR();
    terminal->background_color = terminal->default_background_color;
    terminal->foreground_color = terminal->default_foreground_color;

    // this handler does not read more parameters.
    return 0;
}

int sgr_bold_on_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_bold_on_handler");

    SET_ATTR(BOLD_ATTR);

    // this handler does not read more parameters.
    return 0;
}

int sgr_underscore_on_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_underscore_on_handler");

    SET_ATTR(UNDERSCORE_ATTR);

    // this handler does not read more parameters.
    return 0;
}

int sgr_set_foreground_24bit_color_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_set_foreground_24bit_color_handler");

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

        ASSERT((BETWEEN(parameters[2], 0, 256)), "256 color not in range.\n");

        terminal->foreground_color = parameters[2];

        return 2;
    }

fail:
    return -1;
}

int sgr_set_background_24bit_color_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_set_background_24bit_color_handler");

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

        ASSERT((BETWEEN(parameters[2], 0, 256)), "256 color not in range.\n");

        terminal->background_color = parameters[2];

        return 2;
    }

fail:
    return -1;
}

int sgr_set_foreground_color_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_set_foreground_color_handler");

    if (BETWEEN(parameters[0], 30, 37) ||
        BETWEEN(parameters[0], 90, 97)){
        terminal->foreground_color = parameters[0];
        return 0;
    }

    // setting default
    if (parameters[0] == 39){
        terminal->foreground_color = terminal->default_foreground_color;
        return 0;
    }

    return 0;
}

int sgr_set_background_color_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_set_background_color_handler");

    if (BETWEEN(parameters[0], 40, 47) ||
        BETWEEN(parameters[0], 100, 107)){
        terminal->foreground_color = parameters[0];
        return 0;
    }

    // setting default
    if (parameters[0] == 49){
        terminal->background_color = terminal->default_background_color;
        return 0;
    }

    return 0;
}

int sgr_reverse_video_on_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_reverse_video_on_handler");

    SET_ATTR(REVERSE_ATTR);
    return 0;
}

int sgr_reverse_video_off_handler(Terminal* terminal, int* parameters, int left){
    DEBUG_SGR_HANDLER("sgr_reverse_video_off_handler");

    SET_NO_ATTR(REVERSE_ATTR);
    return 0;
}

int (*sgr_code_handlers[108])(Terminal* terminal, int* parameters, int left) = {
    [0] = sgr_reset_attributes_handler,
    [1] = sgr_bold_on_handler,
    [4] = sgr_underscore_on_handler,

    [7] = sgr_reverse_video_on_handler,
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
    DEBUG_CSI_HANDLER("csi_ich_handler");
}

void csi_cuu_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cuu_handler");

    int len = 0;
    int* parameters = NULL;
    int rows_number = 1;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT((len <= 1), "cuu -> number of parameters is larger than 1.\n");

    if (len == 1){
        rows_number = parameters[0];
    }
    ASSERT((BETWEEN(rows_number, 
                    0, 
                    terminal->cursor.y)),
           "cuu -> number of rows exceed limit.\n");

    terminal->cursor.y -= rows_number;

fail:
    csi_free_parameters(parameters);
}

void csi_cud_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cud_handler");

    int len = 0;
    int* parameters = NULL;
    int rows_number = 1;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT((len <= 1), "cud -> number of parameters is larger than 1.\n");

    if (len == 1){
        rows_number = parameters[0];
    }
    ASSERT((BETWEEN(rows_number, 
                    0, 
                    (terminal->rows_number - 1) - terminal->cursor.y)),
           "cud -> number of rows exceed limit.\n");

    terminal->cursor.y += rows_number;

fail:
    csi_free_parameters(parameters);
}

void csi_cuf_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cuf_handler");

    int len = 0;
    int* parameters = NULL;
    int cols_number = 1;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT((len <= 1), "cuf -> number of parameters is larger than 1.\n");

    if (len == 1){
        cols_number = parameters[0];
    }
    ASSERT((BETWEEN(cols_number, 
                    0, 
                    (terminal->cols_number - 1) - terminal->cursor.x)),
           "cuf -> number of cols exceed limit.\n");

    terminal->cursor.x += cols_number;

fail:
    csi_free_parameters(parameters);
}

void csi_cub_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cub_handler");

    int len = 0;
    int* parameters = NULL;
    int cols_number = 1;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT((len <= 1), "cub -> number of parameters is larger than 1.\n");

    if (len == 1){
        cols_number = parameters[0];
    }
    ASSERT((BETWEEN(cols_number, 
                    0, 
                    terminal->cursor.x)),
           "cub -> number of cols exceed limit.\n");

    terminal->cursor.x -= cols_number;

fail:
    csi_free_parameters(parameters);
}

void csi_cnl_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cnl_handler");
}

void csi_cpl_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cpl_handler");
}

void csi_cha_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cha_handler");
}

void csi_cup_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_cup_handler");

    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL){
        terminal->cursor.x = 0;
        terminal->cursor.y = 0;
        return;
    }

    ASSERT((len == 2), "num of parameters is not equal to 2.\n");

    int row = parameters[0] - 1;
    int col = parameters[1] - 1;

    ASSERT((BETWEEN(row, 0, terminal->rows_number - 1)), "row number is not valid: %d.\n", row);
    ASSERT((BETWEEN(col, 0, terminal->cols_number - 1)), "col number is not valid: %d.\n", col);

    terminal->cursor.x = col;
    terminal->cursor.y = row;

fail:
    csi_free_parameters(parameters);
    return;
}

void csi_ed_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_ed_handler");

    // TODO: make it more readable.

    int ret;
    int len = 0;
    int* parameters = NULL;
    parameters = csi_get_parameters(terminal, &len);
    
    // default: from cursor to end of display
    if ((parameters == NULL) || (parameters[0] == 0)){ // default.
        int i;
        for (i = terminal->cursor.x; i < terminal->cols_number; i++){
            ret = terminal_empty_element(  terminal, 
                                            i, 
                                            terminal->cursor.y);
            ASSERT(ret == 0, "failed to delete element.\n");
        }
        for (i = terminal->cursor.y + 1; i < terminal->rows_number; i++){
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
            ret = terminal_empty_element(  terminal, 
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
    DEBUG_CSI_HANDLER("csi_el_handler");

    int ret;
    int len = 0;
    int* parameters = NULL;
    parameters = csi_get_parameters(terminal, &len);

    // default: from cursor to end of line 
    if ((parameters == NULL) || (parameters[0] == 0)){ // default.
        int i;
        for (i = terminal->cursor.x; i < terminal->cols_number; i++){
            ret = terminal_empty_element(  terminal, 
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
            ret = terminal_empty_element(  terminal, 
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
}

void csi_il_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_il_handler");

    int ret;
    int len = 0;
    int* parameters = NULL;
    int lines_number;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL || parameters[0] == 0){
        lines_number = 1;
    }else{
        ASSERT((len == 1), "csi_il -> number of parameters is: %d\n", len);
        lines_number = parameters[0];
    }

    ASSERT((len <= 1), "too many parameters.\n");

    ret = terminal_scrolldown(  terminal, 
                                terminal->cursor.y, 
                                terminal->bottom,
                                lines_number);
    ASSERT(ret == 0, "failed to scroll down.\n");

fail:
    csi_free_parameters(parameters);
}

void csi_dl_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_dl_handler");

    int ret;
    int len = 0;
    int* parameters = NULL;
    int lines_number;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL || parameters[0] == 0){
        lines_number = 1;
    }else{
        ASSERT((len == 1), "csi_il -> number of parameters is: %d\n", len);
        lines_number = parameters[0];
    }

    ASSERT((len <= 1), "too many parameters.\n");

    ret = terminal_scrollup(    terminal, 
                                terminal->cursor.y, 
                                terminal->bottom,
                                lines_number);
    ASSERT(ret == 0, "failed to scroll up.\n");

fail:
    csi_free_parameters(parameters);
}

void csi_dch_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_dch_handler");
}

void csi_ech_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_ech_handler");

    int chars_number;
    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL || parameters[0] <= 0){
        chars_number = 1; // ther default.
    }else{
        ASSERT((len == 1), "csi_ech_handler number of parameters is: %d\n", len);

        // max
        int max_chars = terminal->cols_number - 1 - terminal->cursor.x;

        ASSERT(BETWEEN(parameters[0], 1, max_chars),
               "the parameter of csi_ech is not in range\n");

        chars_number = parameters[0];
    }

    int ret;
    int i; 
    for (i = 0; i < chars_number; i++){
        ret = terminal_empty_element(   terminal, 
                                        terminal->cursor.x + i,
                                        terminal->cursor.y);
        ASSERT(ret == 0, "csi_ech failed to empty element.\n");
    }

fail:
    csi_free_parameters(parameters);
}

void csi_hpr_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_hpr_handler");
}

void csi_da_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_da_handler");
}

void csi_vpa_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_vpa_handler");
}

void csi_vpr_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_vpr_handler");
}

void csi_hvp_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_hvp_handler");
}

void csi_tbc_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_tbc_handler");
}

void csi_sm_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_sm_handler");

    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL || parameters[0] == 0){
        // ignored
        return;
    }

    ASSERT((len == 1), "number of parameters is not 1?\n");

    if (IS_MODE(PRIVATE_MODE)){
        if (parameters[0] == 1){
            SET_VT_MODE(VT_DECCKM_MODE);
        }
        if (parameters[0] == 2){
            SET_VT_MODE(VT_DECANM_MODE);
        }
        if (parameters[0] == 3){
            SET_VT_MODE(VT_DECCOLM_MODE);
        }
        if (parameters[0] == 4){
            SET_VT_MODE(VT_DECSCLM_MODE);
        }
        if (parameters[0] == 5){
            SET_VT_MODE(VT_DECSCNM_MODE);
        }
        if (parameters[0] == 6){
            SET_VT_MODE(VT_DECOM_MODE);
        }
        if (parameters[0] == 7){
            SET_VT_MODE(VT_DECAWM_MODE);
        }
        if (parameters[0] == 8){
            SET_VT_MODE(VT_DECARM_MODE);
        }
        if (parameters[0] == 9){
            SET_VT_MODE(VT_DECINLM_MODE);
        }
        // any other is ignored.


        SET_NO_MODE(PRIVATE_MODE);
    }else{
        if (parameters[0] == 20){
            SET_VT_MODE(VT_LMN_MODE);
        }
    }

fail:
    csi_free_parameters(parameters);
}

void csi_rm_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_rm_handler");

    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    if (parameters == NULL || parameters[0] == 0){
        // ignored
        return;
    }

    ASSERT((len == 1), "number of parameters is not 1?\n");

    if (IS_MODE(PRIVATE_MODE)){
        if (parameters[0] == 1){
            SET_NO_VT_MODE(VT_DECCKM_MODE);
        }
        if (parameters[0] == 2){
            SET_NO_VT_MODE(VT_DECANM_MODE);
        }
        if (parameters[0] == 3){
            SET_NO_VT_MODE(VT_DECCOLM_MODE);
        }
        if (parameters[0] == 4){
            SET_NO_VT_MODE(VT_DECSCLM_MODE);
        }
        if (parameters[0] == 5){
            SET_NO_VT_MODE(VT_DECSCNM_MODE);
        }
        if (parameters[0] == 6){
            SET_NO_VT_MODE(VT_DECOM_MODE);
        }
        if (parameters[0] == 7){
            SET_NO_VT_MODE(VT_DECAWM_MODE);
        }
        if (parameters[0] == 8){
            SET_NO_VT_MODE(VT_DECARM_MODE);
        }
        if (parameters[0] == 9){
            SET_NO_VT_MODE(VT_DECINLM_MODE);
        }
        // any other is ignored.


        SET_NO_MODE(PRIVATE_MODE);
    }else{
        if (parameters[0] == 20){
            SET_NO_VT_MODE(VT_LMN_MODE);
        }
    }

fail:
    csi_free_parameters(parameters);
}

void csi_sgr_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_sgr_handler");

    int ret;
    int i;
    int len = 0;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);

    // reset attributes when no parameters provided.
    if (parameters == NULL){ 
        sgr_reset_attributes_handler(terminal, NULL, 0);
        return;
    }

    for (i = 0; i < len; i++){
        int left = len - i;

        // to prevent out of bound.
        ASSERT((BETWEEN(parameters[i], 0, 199)),
               "sgr parameter is not in range.\n");

        ASSERT(sgr_code_handlers[parameters[i]], 
               "handler to sgr parameter does not exist: %d.\n", 
                            parameters[i]);

        ret = (sgr_code_handlers[parameters[i]])(terminal,
                                                 &parameters[i],
                                                 left);
        ASSERT((ret >= 0), "handler failed for parameter: %d\n",
                            parameters[i]);

        i += ret; // continue to next parameters.
    }

fail:
    csi_free_parameters(parameters);
}

void csi_dsr_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_dsr_handler");

    int len;
    int* parameters = NULL;

    parameters = csi_get_parameters(terminal, &len);
    ASSERT(parameters, "dsr -> no csi parameters.\n");

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
        ASSERT_TO(fail_on_pty_write, (ret >= 0), "dsr -> failed to write to pty.\n");
    }

fail_on_pty_write:
    csi_free_parameters(parameters);
fail:
    return;
}

void csi_decll_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_decll_handler");
}

void csi_decstbm_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_decstbm_handler");

    int len = 0;
    int* parameters = NULL;

    int top;
    int bottom;

    parameters = csi_get_parameters(terminal, &len);

    // defaults
    if (parameters == NULL){    
        top = 0;
        bottom = terminal->rows_number - 1;
    }else{
        top = parameters[0] ? (parameters[0] - 1) : 0;
        bottom = parameters[1] ? (parameters[1] - 1) : (terminal->rows_number - 1);
    }

    ASSERT(BETWEEN(top, 0, bottom),
           "decstbm -> parameters not in range.\n");
    ASSERT(BETWEEN(bottom, top, terminal->rows_number - 1),
           "decstbm -> parameters not in range.\n");

    terminal->top = top;
    terminal->bottom = bottom;

fail:
    csi_free_parameters(parameters);
}

void csi_save_cursor_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_save_cursor_handler");
}

void csi_restore_cursor_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_restore_cursor_handler");
}

void csi_hpa_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_hpa_handler");
}

void csi_dol_handler(Terminal* terminal){
    DEBUG_CSI_HANDLER("csi_dol_handler");

    SET_MODE(CSI_DOL_MODE);
}

void (*csi_code_handlers[200])(Terminal* terminal) = {
    ['$'] = csi_dol_handler,

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

// ----------------------------------------------------------------------
// Top level handlers
// ----------------------------------------------------------------------

int handle_csi_dol_codes(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(CSI_DOL_MODE)){
        return FALSE;
    }

    if (control_code == 'p'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }
    if (control_code == 'v'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }
    if (control_code == 'z'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }
    if (control_code == 'x'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }
    if (control_code == 'r'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }
    if (control_code == 't'){
        LOG("csi_dol -> handle control_code: %d\n", control_code);
        SET_NO_MODE(CSI_DOL_MODE);
        return TRUE;
    }

    LOG("csi_dol -> failed to handle control_code: %d\n", control_code);
    return TRUE;
}

int handle_osc_codes(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(OSC_MODE)){
        return FALSE;
    }

    // did end?
    if ((control_code == 0x7) || 
        (control_code == '\\')){

        LOG("osc -> string: \"%s\".\n", terminal->osc_buffer);

        // TODO: handle osc commands

        SET_NO_MODE(OSC_MODE);
        SET_NO_MODE(ESC_MODE);

        memset(terminal->osc_buffer, 0, sizeof(terminal->osc_buffer));
        terminal->osc_buffer_index = 0;

        return TRUE;
    }

    ASSERT((terminal->osc_buffer_index + 1 < OSC_MAX_CHARS), 
            "osc -> number of string exceeded limit.\n");

    terminal->osc_buffer[terminal->osc_buffer_index] = control_code;
    terminal->osc_buffer_index++;

    return TRUE;

fail:
    LOG("osc handler failed..?\n");
    return TRUE;
}

int handle_csi_codes(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(CSI_MODE)){
        return FALSE;
    }

    // handle parameters.
    if (BETWEEN(control_code, 0x30, 0x3F)){

        // did we reached the maximum number of parameters
        if (terminal->csi_parameters_index == CSI_MAX_PARAMETERS_CHARS){
            memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
            terminal->csi_parameters_index = 0;

            SET_NO_MODE(CSI_MODE);
            SET_NO_MODE(ESC_MODE);

            LOG("too much parameters.\n");

            return FALSE;
        }
        terminal->csi_parameters[terminal->csi_parameters_index] = control_code;
        terminal->csi_parameters_index++;

        return TRUE;
    }

    // NOTE: these characters will only come after numbers.
    // 0x20 <= control_code <= 0x2F
    // intermediate characters of csi.

    if ((BETWEEN(control_code, 0x40, 0x7E)) ||
        (BETWEEN(control_code, 0x20, 0x2F))){

        if (csi_code_handlers[control_code]){
            (csi_code_handlers[control_code])(terminal);
        }else{
            LOG("csi handler wasn't found for: %d\n", control_code);
            LOG("parameters for debuging: \"%s\".\n", terminal->csi_parameters);
        }

        memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
        terminal->csi_parameters_index = 0;

        SET_NO_MODE(PRIVATE_MODE);
        SET_NO_MODE(CSI_MODE);
        SET_NO_MODE(ESC_MODE);

        return TRUE;
    }
    LOG("csi -> control code is out of range: %d.\n", control_code);
    LOG("csi -> parameters for debuging: \"%s\".\n", terminal->csi_parameters);

    memset(&terminal->csi_parameters, 0, sizeof(terminal->csi_parameters));
    terminal->csi_parameters_index = 0;

    SET_NO_MODE(PRIVATE_MODE);
    SET_NO_MODE(CSI_MODE);
    SET_NO_MODE(ESC_MODE);

    return FALSE;
}

int handle_esc_codes(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(ESC_MODE)){
        return FALSE;
    }

    if (esc_code_handlers[control_code]){
        (esc_code_handlers[control_code])(terminal);

        SET_NO_MODE(ESC_MODE);
        return TRUE;
    }else{
        LOG("no esc handler found for: %u\n", control_code);
    }

    return TRUE;
}

int handle_set_g0_charset(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(G0_MODE)){
        return FALSE;
    }

    if (control_code == 'A'){
        // Set United Kingdom G0 charset
    }
    if (control_code == 'B'){
        // Set United State G0 charset
    }
    if (control_code == '0'){
        // Set G0 special chars. & line set
    }
    if (control_code == '1'){
        // Set G0 alternate character ROM
    }
    if (control_code == '2'){
        // Set G0 alternate character ROM & spec & graphic
    }
    if (control_code == 'U'){
    }
    if (control_code == 'K'){
    }

    SET_NO_MODE(G0_MODE);
    return TRUE;
}

int handle_set_g1_charset(Terminal* terminal, unsigned char control_code){
    if (!IS_MODE(G1_MODE)){
        return FALSE;
    }

    if (control_code == 'A'){
        // Set United Kingdom G1 charset
    }
    if (control_code == 'B'){
        // Set United State G1 charset
    }
    if (control_code == '0'){
        // Set G1 special chars. & line set
    }
    if (control_code == '1'){
        // Set G1 alternate character ROM
    }
    if (control_code == '2'){
        // Set G1 alternate character ROM & spec & graphic
    }
    if (control_code == 'U'){
    }
    if (control_code == 'K'){
    }

    SET_NO_MODE(G1_MODE);
    return TRUE;
}

int handle_control_codes(Terminal* terminal, unsigned char control_code){
    
	/* 
     * FROM ST:
     * 
	 * STR sequence must be checked before anything else
	 * because it uses all following characters until it
	 * receives a ESC, a SUB, a ST or any other C1 control
	 * character.
	 */
    if (handle_osc_codes(terminal, control_code)){
        return TRUE;
    }

    if (handle_set_g0_charset(terminal, control_code)){
        return TRUE;
    }

    if (handle_set_g1_charset(terminal, control_code)){
        return TRUE;
    }

    if (handle_csi_dol_codes(terminal, control_code)){
        return TRUE;
    }

    if (handle_csi_codes(terminal, control_code)){
        return TRUE;
    }

    if (handle_esc_codes(terminal, control_code)){
        return TRUE;
    }

    if (control_code_handlers[control_code]){
        (control_code_handlers[control_code])(terminal);
        return TRUE;
    }

    return FALSE;
}

// ----------------------------------------------------------------------

/*
 * This it the main function of the terminal. it gets 
 * character_code as input and handle control code if they appear.
 */

int terminal_emulate(Terminal* terminal, unsigned int character_code){
    int ret;

    // can be control character only if 1 byte.
    if (BETWEEN(character_code, 0, 0xFF)){
        // LOG("cursor position: x: %d, y: %d\n", terminal->cursor.x, terminal->cursor.y);
        unsigned char control_code = character_code & 0xFF;
        ret = handle_control_codes(terminal, control_code);
        if (ret == TRUE){
            return 0;
        }
    }

    // not a control code:
    // insert simple element to the terminal and moving
    // cursor forward.
    ASSERT(BETWEEN( (&ELEMENT),
                    terminal->screen, 
                    (terminal->screen + (terminal->rows_number * terminal->cols_number * sizeof(TElement)))), 
            "out of bound!\n");
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

TElement* terminal_element(Terminal* terminal, int x, int y){
    TElement* element = NULL;

    element = &terminal->screen[(REAL_Y(y) * terminal->cols_number) + x];

    return element;
}


