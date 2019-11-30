COMPILER=gcc
NAME=SUSE
BIN_PATH=bin

all:	
	rm -rf bin
	mkdir -p $(BIN_PATH)
	$(COMPILER) -g -Wall  -c src/$(NAME).c -o $(BIN_PATH)/$(NAME).o
	$(COMPILER) -g -Wall  -c src/suseDefs.c -o $(BIN_PATH)/suseDefs.o
	$(COMPILER) $(BIN_PATH)/$(NAME).o $(BIN_PATH)/suseDefs.o -g -Wall -o $(BIN_PATH)/$(NAME) -lcommons -lbiblioNOC	

clean:
	rm -rf bin

SUSE:
	mkdir -p $(BIN_PATH)
	$(COMPILER) -g -Wall  -c src/$(NAME).c -o $(BIN_PATH)/$(NAME).o
	$(COMPILER) -g -Wall  -c src/suseDefs.c -o $(BIN_PATH)/suseDefs.o
	$(COMPILER) $(BIN_PATH)/$(NAME).o $(BIN_PATH)/suseDefs.o -g -Wall -o $(BIN_PATH)/$(NAME) -lcommons -lbiblioNOC
		 


