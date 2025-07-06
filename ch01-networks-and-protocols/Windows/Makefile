# ch01-networks-and-protocols/Makefile
# ==============================================================================
.PHONY: \
	all \
	clean
.DELETE_ON_ERROR:
# ******************************************************************************
CC         = gcc
CFLAGS     = -Wall
DBGFLAGS   = -g3 -O0 -DDEBUG
LDFLAGS    = -lws2_32
# ******************************************************************************
MODE      ?= release
# ******************************************************************************
SOURCES    = wsa_init.c win_list.c list_ip.c
MODULES    = $(subst .c,.o,$(SOURCES))
BINARIES   = $(subst .c,.exe,$(SOURCES))
# *****************************************************************************
G_BINARIES = $(subst .c,.dbg.exe,$(SOURCES))
# ******************************************************************************

all: $(BINARIES)

# ********************************************  LINK  **************************
$(word 1,$(BINARIES)): %.exe: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(wordlist 2,3,$(BINARIES)): LDFLAGS += -liphlpapi
$(wordlist 2,3,$(BINARIES)): %.exe: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ********************************************  DEBUG BUILD  *******************
$(word 1,$(G_BINARIES)): %.dbg.exe: %.c
	$(CC) $(CFLAGS) $(DBGFLAGS) $< -o $@ $(LDFLAGS)

$(wordlist 2,3,$(G_BINARIES)): LDFLAGS += -liphlpapi
$(wordlist 2,3,$(G_BINARIES)): %.dbg.exe: %.c
	$(CC) $(CFLAGS) $(DBGFLAGS) $< -o $@ $(LDFLAGS)

# ********************************************  CLEAN UP  **********************
clean:
	rm -fv *.o *.exe

