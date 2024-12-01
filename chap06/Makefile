# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap06.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/web_get

clean:
	rm -rfv $(BINR)/*

$(BINR)/web_get: web_get.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
