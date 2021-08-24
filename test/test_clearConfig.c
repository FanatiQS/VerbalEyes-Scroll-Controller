#include <stdio.h>
#include <string.h>

#include "../verbalEyes_speed_controller.h"

#include "./helpers/conf.h"
#include "./helpers/log.h"
#include "./helpers/print_colors.h"

// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
int8_t verbaleyes_network_connected() { return 0; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
int8_t verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 0; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
char verbaleyes_conf_read(const unsigned short addr) { return 0; }



// Number of errors occuring
int numberOfErrors = 0;

int main() {
	// Clears conf buffer
	conf_clear();

	// Runs through clearConfig script data
	char c;
	while ((c = getc(stdin)) != EOF) {
		verbaleyes_configure(c);
	}

	// Verifies it cleared entire EEPROM
	for (int i = 0; i < CONFIGLEN; i++) {
		if (confBuffer[i] != 0) {
			for (int i = 0; i < CONFIGLEN; i++) {
				printf("%i\t%i\n", i, confBuffer[i]);
			}
			return 0;
		}
	}

	printf("" COLOR_GREEN "\nTest successful: Entire EEPROM can be cleared correctly\n\n" COLOR_NORMAL);

	return 0;
}
