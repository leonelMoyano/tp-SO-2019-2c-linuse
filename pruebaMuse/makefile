UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations -g
endif

CC := gcc
BUILD=bin

all: clean recursiva supermasivo

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin


supermasivo:
	$(CC) src/archivo_de_swap_supermasivo.c -g -L"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug" -Wall $(CFLAGS) -o $(BUILD)/supermasivo -llibMUSE
	
	
recursiva:
	$(CC) src/recursiva.c -g -L"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug" -Wall $(CFLAGS) -o $(BUILD)/recursiva -llibMUSE
