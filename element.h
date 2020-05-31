#ifndef ELEMENT_H
#define ELEMENT_H


typedef struct{
    unsigned int character_code;
    unsigned int attributes;
	unsigned int foreground_color;
	unsigned int background_color;
    unsigned int dirty;
}TElement;


#endif
