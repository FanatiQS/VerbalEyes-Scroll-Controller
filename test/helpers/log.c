#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./print_colors.h"
#include "./log.h"

extern int numberOfErrors;

static size_t logBufferIndex = 0;
static char logBuffer[1024];
static int logFlags = 0;



// Logs VerbalEyes data to console and/or buffer
void verbaleyes_log(const char* msg, const size_t len) {
	// Ensures that log message is expected full length
	if (strlen(msg) != len) {
		fprintf(stderr, "" COLOR_RED "\nMessage to be logged was not the same length as its len argument\n\tstrlen(msg): %zu\n\tlen: %zu\n\tmessage: %s\n" COLOR_NORMAL, strlen(msg), len, msg);
		numberOfErrors++;
		exit(EXIT_FAILURE);
	}

	// Prints log unbuffered to stdout
	if (logFlags & LOGFLAGPRINT) {
		printf("%s", msg);
		fflush(stdout);
	}

	// Copies log to buffer
	if (logFlags & LOGFLAGBUFFER) {
		if (logBufferIndex + strlen(msg) > sizeof(logBuffer)) {
			perror("" COLOR_RED "Too much data logged without clearing\n" COLOR_NORMAL);
			exit(EXIT_FAILURE);
		}
		memcpy(logBuffer + logBufferIndex, msg, len);
		logBufferIndex += len;
	}
}



// Sets logging flags
void log_setflags(const int flags) {
	logFlags = flags;
}

// Compares logged data with argument to check that they match
void log_cmp(const char* source) {
	// Prints success
	if (!strcmp(logBuffer, source)) {
		printf("" COLOR_GREEN "Logs matched\n" COLOR_NORMAL);
		return;
	}

	// Prints failure
	int len = (strlen(source) < strlen(logBuffer)) ? strlen(source) : strlen(logBuffer);
	fprintf(stderr, "" COLOR_RED "Logs did not match:\n%s\n%s\n" COLOR_NORMAL, logBuffer, source);
	fprintf(stderr, "" COLOR_RED "Lengths: %lu %lu\n" COLOR_NORMAL, strlen(source), strlen(logBuffer));
	for (int i = 0; i < len; i++) {
		fprintf(stderr, "index: %d  %d  %d\n", i, logBuffer[i], source[i]);
	}
	fprintf(stderr, "\n");
	numberOfErrors++;
}

// Clears log buffer and starts buffering from the start
void log_clear() {
	memset(logBuffer, 0, sizeof(logBuffer));
	logBufferIndex = 0;
}
