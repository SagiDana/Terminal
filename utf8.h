#ifndef UTF8_H
#define UTF8_H


/*
 * Copied from:
 * https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1


unsigned int utf8_decode(unsigned int* state, 
                         unsigned int* codep, 
                         unsigned int byte);

#endif
