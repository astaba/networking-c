# Make sure bin subfolder exists in the working directory
BINR=./bin
CFLAGS=-Wall -Wextra -I .
DBGFLAGS=-g3 -O0 -DDEBUG
LIBS=
DEPS=chap04.h
TARGET=file_to_debug

.PHONY: all clean debug

all: $(BINR)/udp_client $(BINR)/udp_sendto $(BINR)/udp_recvfrom \
	$(BINR)/udp_serve_toupper $(BINR)/udp_serve_toupper_simple

clean:
	rm -rfv $(BINR)/*

$(BINR)/udp_client: udp_client.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/udp_recvfrom: udp_recvfrom.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/udp_sendto: udp_sendto.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/udp_serve_toupper: udp_serve_toupper.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

$(BINR)/udp_serve_toupper_simple: udp_serve_toupper_simple.c $(DEPS)
	gcc $< -o $@ $(CFLAGS)

debug:
	gcc $(TARGET).c -o $(BINR)/$(TARGET) $(CFLAGS) $(DBGFLAGS)
