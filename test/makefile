EXE=./test
A=gcc ../verbalEyes_speed_controller.c ./lib/bearssl/*.c -Ilib -o $(EXE)

test_config:
	$(A) test_config.c log.c eeprom.c -DCONFIGTIMEOUT=0
	$(EXE)
	rm $(EXE)

