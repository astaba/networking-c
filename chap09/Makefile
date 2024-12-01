# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=-lssl -lcrypto
DEPS=chap09.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/openssl_version $(BINR)/https_simple

clean:
	rm -rfv $(BINR)/*

$(BINR)/openssl_version: openssl_version.c
	gcc $< -o $@ $(CFLAGS) $(LIBS)

$(BINR)/https_simple: https_simple.c $(DEPS)
	gcc $< -o $@ $(CFLAGS) $(LIBS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
