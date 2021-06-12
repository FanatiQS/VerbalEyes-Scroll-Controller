main: ../../test/lib/bearssl
	gcc main.c ../../verbalEyes_speed_controller.c ../../test/lib/bearssl/*.c -I../../test/lib -o emulator

clean:
	rm emulator

../../test/lib/bearssl:
	cd ../../test/lib && make
