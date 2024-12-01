# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap05.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/dns_query $(BINR)/lookup

clean:
	rm -rfv $(BINR)/*

$(BINR)/dns_query: dns_query.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/lookup: lookup.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
