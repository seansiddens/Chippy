EXEC    = chippy

SOURCES = chippy.c stack.c
OBJECTS = $(SOURCES:%.c=%.o)

CC      = clang
CFLAGS  = -I/usr/include/SDL2 -Wall -Wpedantic -Wextra -Ofast
LFLAGS  = -lm -lSDL2

.PHONY: all clean format

all: $(EXEC)


$(EXEC): $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

format:
	clang-format -i -style=file *.c *.h

clean:
	rm -rf $(EXEC) $(OBJECTS)
