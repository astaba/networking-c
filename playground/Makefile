# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS= #-Wall -Wextra
DBGFLAGS=-g3 -O0 -DDEBUG
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/06_web_get

clean:
	rm -rfv $(BINR)/*

# $(BINR)/06_web_get: 06_web_get.c ../chap06/chap06.h
# 	gcc $< -o $@ $(CFLAGS)

$(BINR)/06_web_get: 06_web_get.c
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)

