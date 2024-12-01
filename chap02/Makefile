# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap02.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/sock_init $(BINR)/time_console $(BINR)/time_server \
	$(BINR)/time_server_dual $(BINR)/time_server_ipv6 $(BINR)/time_console_inetd

clean:
	rm -rfv $(BINR)/*

$(BINR)/sock_init: sock_init.c
	gcc $< -o $@ $(CFLAGS)

$(BINR)/time_console: time_console.c
	gcc $< -o $@ $(CFLAGS)

$(BINR)/time_server: time_server.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/time_server_dual: time_server_dual.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/time_server_ipv6: time_server_ipv6.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/time_console_inetd: time_console_inetd.c
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
