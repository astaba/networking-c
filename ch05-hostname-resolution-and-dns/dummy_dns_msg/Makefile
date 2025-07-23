# ch05-hostname-resolution-and-dns/dns_query/Makefile
# ******************************************************************************
.PHONY: \
	all \
	clean
.DELETE_ON_ERROR:
# ******************************************************************************
UNAME      = $(shell uname -s)
IS_MSYS    = $(findstring MSYS_NT,$(UNAME))
# ******************************************************************************
CC         = gcc
CFLAGS     = -Wall -Wextra
DBGFLAGS   = -g3 -O0 -DDEBUG
# LDFLAGS    = -L../../mylib/opt/utility -lutility
LDFLAGS    =
ifeq ($(IS_MSYS),MSYS_NT)
	LDFLAGS += -lws2_32
endif
# ******************************************************************************
vpath %.h ../ ../dns_query/
vpath %.c ../dns_query/
# ******************************************************************************
HEADERS   = chap05.h print_api.h
# ******************************************************************************
SOURCES   = $(wildcard *.c) print_name.c
ifeq ($(IS_MSYS),MSYS_NT)
	BIN_EXT = .exe
	DBG_EXT = .dbg.exe
else
	BIN_EXT = .out
	DBG_EXT = .dbg.out
endif
MODULES   = $(subst .c,.o,$(SOURCES))
BINARY    = $(subst .c,$(BIN_EXT),$(wordlist 1,2,$(SOURCES)))
# ******************************************************************************
G_MODULES = $(subst .c,.dbg.o,$(SOURCES))
G_BINARY  = $(subst .c,$(DBG_EXT),$(wordlist 1,2,$(SOURCES)))
# ******************************************************************************

all: $(BINARY)

# ********************************************  LINK  **************************
$(BINARY): %$(BIN_EXT): %.o print_name.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(G_BINARY): %$(DBG_EXT): %.dbg.o print_name.dbg.o
	$(CC) $^ -o $@ $(LDFLAGS)

# ********************************************  COMPILE AND ASSEMBLE  **********
$(MODULES): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@
$(G_MODULES): %.dbg.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(DBGFLAGS) $< -o $@

# ********************************************  CLEAN UP  **********************
clean:
	rm -fv *.o *$(BIN_EXT)

