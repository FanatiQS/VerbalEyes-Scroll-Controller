EXE = ./tmp
LIB = ../lib
LIBBEARSSL = $(LIB)/bearssl
SRC = ../verbalEyes_speed_controller.c
A = gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE)

all: test_c99 test_config test_clearConfig

$(LIBBEARSSL):
	cd $(LIB) && make

test_config: $(LIBBEARSSL)
	$(A) test_config.c log.c eeprom.c -DCONFIGTIMEOUT=0
	$(EXE)
	rm $(EXE)

test_clearConfig: $(LIBBEARSSL)
	$(A) test_clearConfig.c log.c eeprom.c
	bash ../tools/clearConfig.sh /dev/stdout 1 | $(EXE)
	rm $(EXE)

test_c99: $(LIBBEARSSL)
	gcc -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)

# Standard C++ is currently not supported since VLAs are used
test_c++: $(LIBBEARSSL)
	g++ -S -I$(LIB) -std=c++11 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)
