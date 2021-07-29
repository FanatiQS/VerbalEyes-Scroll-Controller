main: ../../lib/bearssl
	gcc main.c ../../verbalEyes_speed_controller.c ../../lib/bearssl/*.c -I../../lib -o emulator

clean:
	rm emulator

../../lib/bearssl:
	cd ../../lib && make
