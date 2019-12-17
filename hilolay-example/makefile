UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations
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
	$(CC) src/example_program.c -L"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioSuse/Debug" -Wall $(CFLAGS) -o $(BUILD)/$(NAME) -lhilolay -lbiblioSuse
