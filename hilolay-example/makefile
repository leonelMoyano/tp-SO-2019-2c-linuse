UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations -g
endif

CC := gcc
NAME := example
BUILD=bin

all: clean example

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin


example:
	$(CC) src/example_program.c -g -L"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioSuse/Debug" -L"/home/utnso/workspace/tp-2019-2c-No-C-Nada/hilolay/bin" -Wall $(CFLAGS) -o $(BUILD)/$(NAME) -lbiblioSuse -lhilolay 
