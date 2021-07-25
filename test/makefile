EXE=./test
A=gcc ../verbalEyes_speed_controller.c ./lib/bearssl/*.c -Ilib -o $(EXE)

all: test_config test_clearConfig

test_config: lib/bearssl
	$(A) test_config.c log.c eeprom.c -DCONFIGTIMEOUT=0
	$(EXE)
	rm $(EXE)

test_clearConfig: lib/bearssl
	$(A) test_clearConfig.c log.c eeprom.c
	bash ../tools/clearConfig.sh /dev/stdout 1 | $(EXE)
	rm $(EXE)

lib/bearssl/:
	cd ./lib && make

test_c99: lib/bearssl
	gcc -S -Iempty_lib -std=c99 -pedantic ../verbalEyes_speed_controller.c test_c99.c
	rm test_c99.s
