#ifndef COMMON_H
#define COMMON_H

#include <X11/Xlib.h> 
#include <stdlib.h>
#include <stdio.h>

#define FALSE (0)
#define TRUE (!(FALSE))

#define BETWEEN(x, a, b) ((x) >= (a) && ((x) <= (b)))

#define LOG_FILE_PATH "/home/s/terminal.log"

void terminal_log(char* msg);

#define LOG(...) do{                \
    char buff[8128];                 \
    sprintf(buff, __VA_ARGS__);     \
    terminal_log(buff);                   \
}while(0)

#define ASSERT(expr, ...) if(!expr) {LOG(__VA_ARGS__); goto fail;}
#define ASSERT_TO(label, expr, ...) if(!expr) {LOG(__VA_ARGS__); goto label;}

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#endif
