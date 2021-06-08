#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static size_t log_buffer_index = 0;
static char log_buffer[1024];
bool log_print = 0;

void verbaleyes_log(const char* str, const size_t len) {
	// Prints log unbuffered to stdout
	if (log_print) {
		printf("%s", str);
		fflush(stdout);
	}

	// Copies log to buffer
	// memcpy(log_buffer + log_buffer_index, str, len);
	// log_buffer_index += len;
}



void log_reset() {
	memset(log_buffer, 0, sizeof(log_buffer));
	log_buffer_index = 0;
}
