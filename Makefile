SRC      = $(wildcard *.c)
INC      = $(wildcard *.h)
CC       = gcc  #-E

CFLAGS   = `pkg-config --cflags x11 freetype2` \
           -Wall -Wextra \
           #-Wno-unused-parameter

LDFLAGS  = `pkg-config --libs x11 freetype2 xft` \
           -lutil

OBJ = $(SRC:.c=.o)
zt: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) 

$(OBJ): %.o:%.c $(INC) Makefile
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f zt $(OBJ)

