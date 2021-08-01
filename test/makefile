EXE=./test
LIBBEARSSL=../lib/bearssl
A=gcc ../verbalEyes_speed_controller.c $(LIBBEARSSL)/*.c -I../lib -o $(EXE)

all: test_c99 test_config test_clearConfig

$(LIBBEARSSL):
	cd ../lib && make

test_config: $(LIBBEARSSL)
	$(A) test_config.c log.c eeprom.c -DCONFIGTIMEOUT=0
	$(EXE)
	rm $(EXE)

test_clearConfig: $(LIBBEARSSL)
	$(A) test_clearConfig.c log.c eeprom.c
	bash ../tools/clearConfig.sh /dev/stdout 1 | $(EXE)
	rm $(EXE)

test_c99: $(LIBBEARSSL)
	gcc -S -I../lib -std=c99 -pedantic-errors ../verbalEyes_speed_controller.c
	clang -S -I../lib -std=c99 -pedantic-errors ../verbalEyes_speed_controller.c
	rm verbalEyes_speed_controller.s

# Standard C++ is currently not supported since VLAs are used
test_c++: $(LIBBEARSSL)
	g++ -S -I../lib -std=c++11 -pedantic-errors ../verbalEyes_speed_controller.c
