# Make sure bin subfolder exists in the working directory
CFLAGS= -Wall -Wextra
GDBFLAGS= -g3 -O0 -DDEBUG
HDR_F = chap02.h

SRC = sock_init.c \
	time_console.c \
	time_server.c \
	time_server_dual.c \
	time_server_ipv6.c \
	time_console_inetd.c
EXECS = $(SRC:.c=.out)
DEBUG = $(SRC:.c=_dbg.out)

.PHONY: all clean

all: $(EXECS)

$(word 1, $(EXECS)): $(word 1, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 2, $(EXECS)): $(word 2, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 3, $(EXECS)): $(word 3, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 4, $(EXECS)): $(word 4, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 5, $(EXECS)): $(word 5, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 6, $(EXECS)): $(word 6, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $< -o $@

$(word 1, $(DEBUG)): $(word 1, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

$(word 2, $(DEBUG)): $(word 2, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

$(word 3, $(DEBUG)): $(word 3, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

$(word 4, $(DEBUG)): $(word 4, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

$(word 5, $(DEBUG)): $(word 5, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

$(word 6, $(DEBUG)): $(word 6, $(SRC)) $(HDR_F)
	$(CC) $(CFLAGS) $(GDBFLAGS) $< -o $@

clean:
	rm -rfv *.out

