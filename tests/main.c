#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../verbalEyes_speed_controller.h"

int testState;
int testingNetwork;
int testingSocket;

char eeprom[CONFIGLEN];



// Reads a character from the specified address in EEPROM
char verbaleyes_conf_read(const unsigned short addr) {
  return eeprom[addr];
}

// Writes a character to the specified address in EEPROM
void verbaleyes_conf_write(const unsigned short addr, const char c) {

}

// Commits changes made in EEPROM to flash
void verbaleyes_conf_commit() {

}



// Connects to a WiFi network
void verbaleyes_network_connect(const char* ssid, const char* key) {
	switch (testState) {
		case 1: {
			if (testingNetwork == 0) {
				testingNetwork = 1;
			}
			else if (testingNetwork == 1 && testingSocket == 1) {
				testingNetwork = 2;
			}
			else {
				printf("\nERROR: verbaleyes_network_connect was called a third time\n");
				exit(1);
			}
			return;
		}
		default: {
			if (testingNetwork != 0) {
				printf("\nERROR: verbaleyes_network_connect was called a second time\n");
				exit(1);
			}
			testingNetwork = 1;
		}
	}
}
bool verbaleyes_network_connected() {
	switch (testState) {
		case 0: return 0;
		case 1: {
			if (testingSocket == 0) {
				return 1;
			}
			else {
				return 0;
			}
		}
		default: return 1;
	}
}

// Gets the local ip address for printing
uint32_t verbaleyes_network_getip() {
  return 1;
}



// Connects the socket to an endpoint
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
	switch (testState) {
		case 0: {
			printf("\nERROR: Unexpectedly got to verbaleyes_socket_connect while testing network connection timeout\n");
			exit(1);
		}
		case 1: {
			if (testingSocket != 0) {
				printf("\nERROR: verbaleyes_network_connect was called a second time\n");
				exit(1);
			}
			testingSocket = 1;
			return;
		}
	}
}

// Gets the connection status of the socket connection
bool verbaleyes_socket_connected() {
  switch (testState) {
	case 0: {
		printf("\nERROR: Unexpectedly got to verbaleyes_socket_connected while testing network connection timeout\n");
		exit(1);
	}
  	case 1: {
		return 0;
	}
  }
}

// Consumes a single character from the sockets response data buffer
short verbaleyes_socket_read() {
  return 1;
}

// Sends a string to the endpoint the socket is connected to
void verbaleyes_socket_write(const char* str, const unsigned int len) {

}



// Prints a string to the serial interface
void verbaleyes_log(const char* str, const uint32_t len) {
  printf("%s", str);
  fflush(stdout);
}




int main(void) {
	strcpy(eeprom + 0, "myWiFi");
	strcpy(eeprom + 32, "pass123");
	strcpy(eeprom + 64, "192.168.1.1");
	eeprom[128] = 0;
	eeprom[129] = 80;

	testState = 0;
	testingNetwork = 0;
	printf("\n\nTest network timeout\n===");
	while (1) {
		const int state = ensureConnection();
		if (state == CONNECTING) continue;
		if (state == CONNECTIONFAILED) break;
		printf("\nERROR: Got unexpected connection state %i\n", state);
		exit(1);
	}
	if (ensureConnection() != CONNECTING) {
		printf("\nERROR: Got unexpected connection state\n");
		exit(1);
	}
	printf("\n===\nTest successfull\n");

	testState = 1;
	testingNetwork = 0;
	testingSocket = 0;
	printf("\n\nTest network lost\n===");
	while (1) {
		const int state = ensureConnection();
		if (testingNetwork == 2) break;
		if (state == CONNECTING) continue;
		printf("\nERROR: Got unexpected connection state %i\n", state);
		exit(1);
	}
	printf("\n===\nTest successfull\n");

	testState = 2;
	testingNetwork = 0;
	testingSocket = 0;
	printf("\n\nTest socket timeout\n===");
	while (1) {
		const int state = ensureConnection();
		if (state == CONNECTING) continue;
		if (state == CONNECTIONFAILED) break;
		printf("\nERROR: Got unexpected connection state %i\n", state);
		exit(1);
	}
	if (ensureConnection() != CONNECTING) {
		printf("\nERROR: Got unexpected connection state\n");
		exit(1);
	}
	printf("\n===\nTest successfull\n");

	return 0;
}


// Need test for retry blocking
