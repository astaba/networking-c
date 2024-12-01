# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS= #-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/unix_list

clean:
	rm -rfv $(BINR)/*

$(BINR)/unix_list: unix_list.c
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
