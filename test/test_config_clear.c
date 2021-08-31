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



// Number of errors occuring
int numberOfErrors = 0;

int main() {
	// Clears conf buffer
	conf_clear();

	// Prints logs
	log_setflags(LOGFLAGPRINT);

	// Opens configuration file to test
	FILE* file = fopen("../tools/configure_bash/config_clear", "r");

	// Throws on file unable to be opened
	if (file == NULL) {
		fprintf(stderr, "Unable to read file\n");
		exit(EXIT_FAILURE);
	}

	// Runs through file data
	char c;
	while ((c = fgetc(file)) != EOF) {
		verbaleyes_configure(c);
	}

	// Verifies it cleared entire conf buffer and commited it
	printf("\n");
	printf("" COLOR_BLUE "Testing config_clear script:\n" COLOR_NORMAL);
	char buf[CONFIGLEN];
	memset(buf, '0', CONFIGLEN);
	conf_cmp(0, buf, CONFIGLEN);
	conf_matchcommit(1);
	printf("" COLOR_GREEN "Test successful: Entire configuration can be cleared correctly\n\n" COLOR_NORMAL);

	return 0;
}
