X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

LIBS = -L${X11LIB} -lX11 -lXft -lutil \
	   `pkg-config --libs freetype2` \
	   `pkg-config --libs fontconfig` 

INCS = -I${X11INC} \
	   `pkg-config --cflags freetype2` \
	   `pkg-config --libs fontconfig` 

CC = cc

LDFLAGS = ${LIBS}
CFLAGS = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os ${INCS}

SRC = ui.c terminal.c pty.c common.c list.c element.c font.c utf8.c color.c

OBJ = ${SRC:.c=.o}

all: t options

options:
	@echo wm build options:
	@echo "CFLAGS	=${CFLAGS}"
	@echo "LDFLAGS	=${LDFLAGS}"
	@echo "CC	=${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

t: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean: 
	rm t *.o
