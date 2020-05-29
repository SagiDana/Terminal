#ifndef PTY_H
#define PTY_H


typedef struct{
    int master;
}TPty;


TPty* pty_create(char** args, char* terminal_name);
void pty_destroy(TPty* pty);

int pty_read(   TPty* pty, 
                char* buf,
                unsigned int len);

int pty_write(  TPty* pty,
                char* buf,
                unsigned int len);

int pty_resize( TPty* pty,
                int cols_number,
                int rows_number);

int pty_pending(TPty* pty);

#endif
