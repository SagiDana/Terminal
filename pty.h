#ifndef PTY_H
#define PTY_H


typedef struct{
    int master;
}TPty;


TPty* pty_create(char** args);
void pty_destroy(TPty* pty);

int pty_read(   TPty* pty, 
                char* buf,
                unsigned int len);
int pty_write(  TPty* pty,
                char* buf,
                unsigned int len);

int pty_pending(TPty* pty);

#endif
