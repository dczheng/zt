SRC      = $(wildcard *.c)
INC      = $(wildcard *.h)
CC       = gcc  #-E
CFLAGS   = -I/usr/local/include \
           -I/usr/local/include/freetype2 \
           -Wall -Wextra \
           -Wno-deprecated-declarations \
           #-Wno-unused-parameter
LDFLAGS  = -L/usr/local/lib -lX11 -lXft \
           -lXinerama -lutil

OBJ = $(SRC:.c=.o)
zt: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) 

$(OBJ): %.o:%.c $(INC) Makefile
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f zt $(OBJ)

