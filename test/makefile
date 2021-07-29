EXE=./test
LIBBEARSSL=../lib/bearssl
A=gcc ../verbalEyes_speed_controller.c $(LIBBEARSSL)/*.c -I../lib -o $(EXE)

all: test_config test_clearConfig

test_config: $(LIBBEARSSL)
	$(A) test_config.c log.c eeprom.c -DCONFIGTIMEOUT=0
	$(EXE)
	rm $(EXE)

test_clearConfig: $(LIBBEARSSL)
	$(A) test_clearConfig.c log.c eeprom.c
	bash ../tools/clearConfig.sh /dev/stdout 1 | $(EXE)
	rm $(EXE)

$(LIBBEARSSL):
	cd ../lib && make

test_c99: $(LIBBEARSSL)
	gcc -S -I../lib -std=c99 -pedantic-errors ../verbalEyes_speed_controller.c
	clang -S -I../lib -std=c99 -pedantic-errors ../verbalEyes_speed_controller.c
	rm verbalEyes_speed_controller.s
