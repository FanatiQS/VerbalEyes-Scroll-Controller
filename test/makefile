EXE = ./tmp
LIB = ../lib
LIBBEARSSL = $(LIB)/bearssl
SRC = ../verbalEyes_speed_controller.c
A = gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE) ./helpers/*.c

all: test_c99 test_c++ test_configure test_config_clear test_perf

$(LIBBEARSSL):
	cd $(LIB) && make

test_configure: $(LIBBEARSSL)
	$(A) test_configure.c -DCONFIGTIMEOUT=1
	$(EXE)
	rm $(EXE)

test_config_clear: $(LIBBEARSSL)
	$(A) test_config_clear.c
	$(EXE)
	rm $(EXE)

test_c99: $(LIBBEARSSL)
	gcc -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)

test_c++: $(LIBBEARSSL)
	g++ -S -I$(LIB) -std=c++11 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)

test_perf:
	gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE) test_perf.c
	$(EXE)
	rm $(EXE)
