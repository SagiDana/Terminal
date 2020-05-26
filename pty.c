#include "pty.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pty.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>


// ------------------------------------------------------------------------
// signal handlers
// ------------------------------------------------------------------------

void sigchld_handler(int arg){
	exit(0);
}

// ------------------------------------------------------------------------

TPty* pty_create(char** args){
    TPty* pty = NULL;

    pty = (TPty*) malloc(sizeof(TPty));
    ASSERT(pty, "failed to malloc() pty.\n");

    int master, slave;
    int ret;

	ret = openpty(&master, &slave, NULL, NULL, NULL);
    if (ret != 0){
        LOG("failed to open pty.\n");
        free(pty);
        pty = NULL;
        goto fail;
    }

    ret = fork();
    if (ret == -1){
        LOG("failed to fork().\n");
        free(pty);
        pty = NULL;
        goto fail;
    }

    // child
    if (ret == 0){
        // create a new process group
		setsid(); 

		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);

		close(slave);
		close(master);

        // TODO: setting environment vairables.
        const struct passwd *pw;

        pw = getpwuid(getuid());
        if (pw == NULL){
            LOG("failed on getpwuid().\n");
            exit(0);
        }

        unsetenv("COLUMNS");
        unsetenv("LINES");
        unsetenv("TERMCAP");

        setenv("LOGNAME", pw->pw_name, 1);
        setenv("USER", pw->pw_name, 1);
        setenv("SHELL", args[0], 1);
        setenv("HOME", pw->pw_dir, 1);
        // setenv("TERM", "terminal", 1);

        // setting signal handlers to defaults.
        signal(SIGCHLD, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGALRM, SIG_DFL);

        execvp(args[0], args);
        exit(1);
    }

    // parent
    if (ret != 0){
        close(slave);

		signal(SIGCHLD, sigchld_handler);

        pty->master = master;
    }

fail:
    return pty;
}

void pty_destroy(TPty* pty){
    free(pty);
}


int pty_read(   TPty* pty, 
                char* buf,
                unsigned int len){
    int ret;

    ret = read(pty->master, buf, len);
    ASSERT((ret >= 0), "failed to read from pty.\n");

    return ret;

fail:
    return -1;
}

int pty_write(  TPty* pty,
                char* buf,
                unsigned int len){
    int ret;

    ret = write(pty->master, buf, len);
    ASSERT((ret >= 0), "failed to read from pty.\n");

    return ret;

fail:
    return -1;
}

int pty_resize( TPty* pty,
                int cols_number,
                int rows_number){

    struct winsize window_size = {
        .ws_col = cols_number,
        .ws_row = rows_number
    };

    ioctl(  pty->master, 
            TIOCSWINSZ, 
            &window_size);

    return 0;
}

int pty_pending(TPty* pty){
    int ret;

    fd_set read_fds;

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 100
    };

    FD_ZERO(&read_fds);
    FD_SET(pty->master, &read_fds);

    ret = select(   pty->master + 1,
                    &read_fds,
                    NULL,
                    NULL,
                    &timeout);
    if (ret == 0){
        return FALSE;
    }
    return TRUE;
}
