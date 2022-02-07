EXE = ./tmp
LIB = lib
LIBBEARSSL = $(LIB)/bearssl
SRC = ../src/scroll_controller.c
A = gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE) ./helpers/*.c

all: test_c test_c++ test test_init

$(LIBBEARSSL):
	cd $(LIB) && make

test: $(LIBBEARSSL)
	$(A) test.c -DCONFIGTIMEOUT=2
	$(EXE)
	rm $(EXE)

test_init: $(LIBBEARSSL)
	$(A) test_init.c -DCONNECTINGTIMEOUT=1 -DCONNECTIONFAILEDDELAY=1
	$(EXE)
	rm $(EXE)

test_config_clear: $(LIBBEARSSL)
	$(A) test_config_clear.c
	$(EXE)
	rm $(EXE)

test_c: $(LIBBEARSSL)
	gcc -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c99 -pedantic-errors $(SRC) -o $(EXE)
	gcc -S -I$(LIB) -std=c11 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c11 -pedantic-errors $(SRC) -o $(EXE)
	gcc -S -I$(LIB) -std=c17 -pedantic-errors $(SRC) -o $(EXE)
	clang -S -I$(LIB) -std=c17 -pedantic-errors $(SRC) -o $(EXE)
	rm $(EXE)

test_c++: $(LIBBEARSSL)
	cp $(SRC) $(SRC).cpp
	g++ -S -I$(LIB) -std=c++98 -pedantic-errors $(SRC).cpp -o $(EXE)
	clang++ -S -I$(LIB) -std=c++98 -pedantic-errors $(SRC).cpp -o $(EXE)
	g++ -S -I$(LIB) -std=c++11 -pedantic-errors $(SRC).cpp -o $(EXE)
	clang++ -S -I$(LIB) -std=c++11 -pedantic-errors $(SRC).cpp -o $(EXE)
	g++ -S -I$(LIB) -std=c++14 -pedantic-errors $(SRC).cpp -o $(EXE)
	clang++ -S -I$(LIB) -std=c++14 -pedantic-errors $(SRC).cpp -o $(EXE)
	g++ -S -I$(LIB) -std=c++17 -pedantic-errors $(SRC).cpp -o $(EXE)
	clang++ -S -I$(LIB) -std=c++17 -pedantic-errors $(SRC).cpp -o $(EXE)
	rm $(SRC).cpp
	rm $(EXE)

test_perf:
	gcc $(SRC) $(LIBBEARSSL)/*.c -I$(LIB) -o $(EXE) test_perf.c
	$(EXE)
	rm $(EXE)
