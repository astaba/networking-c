# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap03.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/tcp_client $(BINR)/tcp_server_chat $(BINR)/tcp_server_toupper \
	$(BINR)/tcp_server_toupper_fork

clean:
	rm -rfv $(BINR)/*

$(BINR)/tcp_client: tcp_client.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/tcp_server_chat: tcp_server_chat.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/tcp_server_toupper: tcp_server_toupper.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/tcp_server_toupper_fork: tcp_server_toupper_fork.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
