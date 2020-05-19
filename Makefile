X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

LIBS = -L${X11LIB} -lX11 -lXft \
	   `pkg-config --libs freetype2`

INCS = -I${X11INC} \
	   `pkg-config --cflags freetype2`

CC = cc

LDFLAGS = ${LIBS}
CFLAGS = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os ${INCS}

SRC = terminal.c common.c list.c

OBJ = ${SRC:.c=.o}

all: terminal options

options:
	@echo wm build options:
	@echo "CFLAGS	=${CFLAGS}"
	@echo "LDFLAGS	=${LDFLAGS}"
	@echo "CC	=${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

terminal: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean: 
	rm terminal *.o
