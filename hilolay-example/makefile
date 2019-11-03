UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations
endif

CC := gcc
NAME := hilolayex
BUILD=bin

all: clean example

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin


example:
	$(CC) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/example src/example_program.c -lhilolay