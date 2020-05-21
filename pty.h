#ifndef PTY_H
#define PTY_H


typedef struct{
    int master;
}Pty;


Pty* pty_create(char** args);
void pty_destroy(Pty* pty);

int pty_read(   Pty* pty, 
                char* buf,
                unsigned int len);
int pty_write(  Pty* pty,
                char* buf,
                unsigned int len);

int pty_pending(Pty* pty);

#endif
