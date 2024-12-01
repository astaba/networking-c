# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap07.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/web_server.alt $(BINR)/web_server $(BINR)/web_server2

clean:
	rm -rfv $(BINR)/*

$(BINR)/web_server.alt: web_server.alt.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/web_server: web_server.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/web_server2: web_server2.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
