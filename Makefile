PROJECT_NAME=socketcanx
CC=gcc
LDLIBS=
CFLAGS=-Wall -Wextra -pedantic -std=c99 -Ix

# Common library object
LIB_OBJ=x/can/lib.o

all: canxsend canxrecv

# Compile the library code once
$(LIB_OBJ): x/can/lib.c x/can/lib.h x/can/lib/can.h
	$(CC) $(CFLAGS) -c x/can/lib.c -o $(LIB_OBJ)

canxsend: canxsend.c $(LIB_OBJ)
	$(CC) $(CFLAGS) canxsend.c $(LIB_OBJ) -o canxsend $(LDLIBS)

canxrecv: canxrecv.c $(LIB_OBJ)
	$(CC) $(CFLAGS) canxrecv.c $(LIB_OBJ) -o canxrecv $(LDLIBS)

clean:
	rm -f canxsend canxrecv $(LIB_OBJ) *.o
