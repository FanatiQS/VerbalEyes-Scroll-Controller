#include <stdio.h>
#include <string.h>

#include "../verbalEyes_speed_controller.h"

#include "./eeprom.h"
#include "./log.h"



// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
int8_t verbaleyes_network_connected() { return 0; }
uint32_t verbaleyes_network_getip() { return 0; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
int8_t verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 0; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
unsigned char verbaleyes_conf_read(const unsigned short addr) { return 0; }



int main() {
	// Sets eeprom to be full of data
	memset(conf_eeprom, 1, CONFIGLEN);

	// Runs through clearConfig script data
	char c;
	while ((c = getc(stdin)) != EOF) {
		updateConfig(c);
	}

	// Verifies it cleared entire EEPROM
	for (int i = 0; i < CONFIGLEN; i++) {
		if (conf_eeprom[i] != 0) {
			for (int i = 0; i < CONFIGLEN; i++) {
				printf("%i\t%i\n", i, conf_eeprom[i]);
			}
			return 0;
		}
	}

	printf("\n\x1b[32mTest successful: Entire EEPROM can be cleared correctly\n\n\x1b[0m");

	return 0;
}
