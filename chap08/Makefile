# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap08.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/smtp_send

clean:
	rm -rfv $(BINR)/*

$(BINR)/smtp_send: smtp_send.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
