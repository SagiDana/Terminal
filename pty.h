#ifndef PTY_H
#define PTY_H


typedef struct{
    int pty_master;
}Pty;


Pty* pty_create(char** args);
void pty_destroy(Pty* pty);

#endif
