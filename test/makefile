EXE = ./tmp
LIB = ../lib
LIBBEARSSL = $(LIB)/bearssl
SRC = ../verbalEyes_speed_controller.c
A = gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE) ./helpers/*.c

all: test_c99 test_configure test_clearConfig

$(LIBBEARSSL):
	cd $(LIB) && make

test_configure: $(LIBBEARSSL)
	$(A) test_configure.c -DCONFIGTIMEOUT=1
	$(EXE)
	rm $(EXE)

test_clearConfig: $(LIBBEARSSL)
	$(A) test_clearConfig.c
	bash ../tools/clearConfig.sh /dev/stdout 0 | $(EXE)
	rm $(EXE)

test_c99: $(LIBBEARSSL)
	gcc -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)

# Standard C++ is currently not supported since VLAs are used
test_c++: $(LIBBEARSSL)
	g++ -S -I$(LIB) -std=c++11 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)
