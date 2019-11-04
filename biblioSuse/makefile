UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations
endif

ifeq ($(UNAME_S),Darwin)
  USR_PATH := /usr/local
else
  USR_PATH := /usr
endif

CC := gcc
MAJOR := 0
MINOR := 1
NAME := biblioSuse
BUILD=bin

all: clean $(NAME).so install

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin

$(NAME).o:
	$(CC) -c -Wall $(CFLAGS) -fpic src/biblioSuse.c -o $(BUILD)/$(NAME).o -lhilolay -lcommons
	$(CC) -c -Wall $(CFLAGS) -fpic src/libSuseUtils.c -o $(BUILD)/libSuseUtils.o -lhilolay -lcommons

$(NAME).so: $(NAME).o
	$(CC) -shared -o $(BUILD)/lib$(NAME).so $(BUILD)/$(NAME).o $(BUILD)/libSuseUtils.o -lhilolay -lcommons
	
install:
	mkdir -p $(USR_PATH)/include/biblioSuse/
	cp src/biblioSuse.h $(USR_PATH)/include/biblioSuse/biblioSuse.h
	cp src/libSuseUtils.h $(USR_PATH)/include/biblioSuse/libSuseUtils.h
	chmod 0755 $(USR_PATH)/include/biblioSuse/*
	cp $(BUILD)/lib$(NAME).so $(USR_PATH)/lib/lib$(NAME).so
