# ch04-establishing-udp-connections/Makefile
# ==============================================================================
.PHONY: \
	all \
	clean
.DELETE_ON_ERROR:
# ******************************************************************************
UNAME      = $(shell uname -s)
IS_MSYS    = $(findstring MSYS_NT,$(UNAME))
# ******************************************************************************
CC         = gcc
# CFLAGS     = -Wall -Wextra
CFLAGS     =
DBGFLAGS   = -g3 -O0 -DDEBUG
LDFLAGS    =
ifeq ($(IS_MSYS),MSYS_NT)
	LDFLAGS += -lws2_32
endif
# ******************************************************************************
MODE      ?= release
# ******************************************************************************
HEADER     = chap04.h
SOURCES    = $(wildcard *.c)
ifeq ($(IS_MSYS),MSYS_NT)
	BIN_EXT = .exe
	DBG_EXT = .dbg.exe
else
	BIN_EXT = .out
	DBG_EXT = .dbg.out
endif
BINARIES   = $(subst .c,$(BIN_EXT),$(SOURCES))
G_BINARIES = $(subst .c,$(DBG_EXT),$(SOURCES))
# ******************************************************************************

all: $(BINARIES)

# ********************************************  COMPILE AND LINK  **************
$(BINARIES): %$(BIN_EXT)  : %.c $(HEADER)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
$(G_BINARIES): %$(DBG_EXT)  : %.c $(HEADER)
	$(CC) $(CFLAGS) $(DBGFLAGS) $< -o $@ $(LDFLAGS)

# ******************************************************************************
clean:
	rm -fv *.o *$(BIN_EXT)

