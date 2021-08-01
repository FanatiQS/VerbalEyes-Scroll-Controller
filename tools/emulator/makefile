LIB=../../lib

main: $(LIB)/bearssl
	gcc main.c ../../verbalEyes_speed_controller.c $(LIB)/bearssl/*.c -I$(LIB) -o emulator

clean:
	rm emulator

$(LIB)/bearssl:
	cd $(LIB) && make
