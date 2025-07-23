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
LDFLAGS    = -L../../mylib/opt/utility -lutility
ifeq ($(IS_MSYS),MSYS_NT)
	LDFLAGS += -lws2_32
endif
# ******************************************************************************
vpath %.h ../ ../../mylib/
HEADERS   = chap05.h print_api.h omniplat.h
# ******************************************************************************
SOURCES   = \
			dns_query.c \
			print_dns_msg.c \
			print_name.c
ifeq ($(IS_MSYS),MSYS_NT)
	BIN_EXT = .exe
	DBG_EXT = .dbg.exe
else
	BIN_EXT = .out
	DBG_EXT = .dbg.out
endif
MODULES   = $(subst .c,.o,$(SOURCES))
BINARY    = $(subst .c,$(BIN_EXT),$(word 1, $(SOURCES)))
# ******************************************************************************
G_MODULES = $(subst .c,.dbg.o,$(SOURCES))
G_BINARY  = $(subst .c,$(DBG_EXT),$(word 1, $(SOURCES)))
# ******************************************************************************

all: $(BINARY)

# ********************************************  LINK  **************************
$(BINARY): $(MODULES)
	$(CC) $^ -o $@ $(LDFLAGS)
$(G_BINARY): $(G_MODULES)
	$(CC) $^ -o $@ $(LDFLAGS)

# ********************************************  COMPILE AND ASSEMBLE  **********
$(MODULES): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@
$(G_MODULES): %.dbg.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $(DBGFLAGS) $< -o $@

# ********************************************  CLEAN UP  **********************
clean:
	rm -fv *.o *$(BIN_EXT)

