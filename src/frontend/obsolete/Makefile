# Makefile for DAPHNE Front-End
# Written by Matt Ownby

# TODO: Add dependencies

CC = gcc
CFLAGS = `gtk-config --cflags`

LIBS = `gtk-config --libs`

OBJS = main.o callbacks.o support.o interface.o

PROGS =	daphnefe

.c.o:
		${CC} ${CFLAGS} -c $<

all:		${PROGS}

daphnefe:	${OBJS}
		${CC} ${CFLAGS} -o daphnefe ${OBJS} ${LIBS}
clean:
		rm -f daphnefe *.o
