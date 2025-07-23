# networking/mylib/Makefile
# Description: Build utility static library

.PHONY: all clean
.DELETE_ON_ERROR:

CC = gcc

HEADERS := omniplat.h
SOURCES := utility.c
MODULES := $(subst .c,.o,$(SOURCES))

RELATIVE_ROOT = ./opt
STATIC_LIB_NAME = utility
STATIC_LIB_DIR = $(RELATIVE_ROOT)/${STATIC_LIB_NAME}
# As relative path
STATIC_LIB = $(STATIC_LIB_DIR)/lib$(STATIC_LIB_NAME).a

all: $(STATIC_LIB)

$(STATIC_LIB): $(MODULES)
	mkdir -p $(@D)
	ar crs $@ $^

$(MODULES): %.o: %.c $(HEADERS)
	$(CC) -c $<

clean:
	@rm -rfv $(RELATIVE_ROOT) *.o
