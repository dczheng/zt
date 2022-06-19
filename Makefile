SRC      = $(wildcard *.c)
INC      = $(wildcard *.h)
CC       = gcc #-E

DEPS     = x11 freetype2 xft fontconfig

CFLAGS   = `pkg-config --cflags $(DEPS)` \
           -Wall -Wextra \
           #-Wno-unused-parameter

LDFLAGS  = `pkg-config --libs $(DEPS)` \
           -lutil

OBJ = $(SRC:.c=.o)
zt: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) 

$(OBJ): %.o:%.c $(INC) Makefile
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f zt $(OBJ)
