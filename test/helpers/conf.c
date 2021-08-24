#include <stdio.h>
#include <string.h>
#include <../verbalEyes_speed_controller.h>
#include "./print_colors.h"
#include "./conf.h"

extern int numberOfErrors;

char confBuffer[CONFIGLEN];
static bool confCommited = 0;
static int confFlags = 0;

#define CLEARBYTE 3



// VerbalEyes function to write data to eeprom
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	// Ensures address is not outside max config length
	if (addr >= CONFIGLEN || addr < 0) {
		fprintf(stderr, "" COLOR_RED "Configuration tried to write outide address range: %d\n" COLOR_NORMAL, addr);
		numberOfErrors++;
		exit(EXIT_FAILURE);
	}

	//!!
	if (confFlags & CONFFLAGCOMMITED && confBuffer[addr] != CLEARBYTE) {
		fprintf(stderr, "" COLOR_RED "Attempt to overwrite conf addr that has already been written to: %d\n" COLOR_NORMAL, addr);
		numberOfErrors++;
		exit(EXIT_FAILURE);
	}
	confBuffer[addr] = c;
}

// VerbalEyes function to commit data to eeprom
void verbaleyes_conf_commit() {
	confCommited = 1;
}



// Updates config with a string instead of a character
bool updateConfig(const char* str) {
	int i = 0;
	if (strlen(str) == 0) return 0;
	do {
		if (!verbaleyes_configure(str[i])) return 1;
	} while (str[i++] != '\n');
	return 0;
}

// Tests that all data within span is cleared
static bool ensureEmpty(int i, int len) {
	bool gotError = 0;
	while ((i < len)) {
		if (confBuffer[i] != CLEARBYTE) {
			if (!gotError) {
				fprintf(stderr, "" COLOR_RED "Configuration buffer was updated at an address where it should not have been:\n" COLOR_NORMAL);
				numberOfErrors++;
			}
			fprintf(stderr, "index: %d \tcode: %d \tchar: %c \n", i, confBuffer[i], confBuffer[i]);
			gotError = 1;
		}
		i++;
	}
	return !gotError;
}

// Sets configuration flags
void conf_setflags(const int flags) {
	confFlags = flags;
}

// Compares saved configuration with argument to check that they matchs
void conf_cmp(const int offset, const char* compareTo, const int len) {
	// Prints buffer mismatch error if compare and buffer content do not match
	if (memcmp(confBuffer + offset, compareTo, len)) {
		fprintf(stderr, "" COLOR_RED "Configuration buffer did not match:\n" COLOR_NORMAL);
		numberOfErrors++;
		for (int i = 0; i < len; i++) {
			fprintf(stderr,
				"index: %d  %u  %u\n",
				i + offset,
				(unsigned char)confBuffer[i + offset],
				(unsigned char)compareTo[i]
			);
		}
	}
	else {
		printf("" COLOR_GREEN "Configuration buffer matched\n" COLOR_NORMAL);
	}

	// Checks if config buffer was updated where it was not expected to
	if (confFlags & CONFFLAGOUTSIDESTR) {
		if (
			ensureEmpty(0, offset) ||
			ensureEmpty(offset + len, CONFIGLEN)
		) {
			printf("" COLOR_GREEN "Configuration buffer was only updated within its range\n" COLOR_NORMAL);
		}
	}
}

// Tests commit state
void conf_matchcommit(bool commitState) {
	if (confCommited != commitState) {
		fprintf(stderr, "" COLOR_RED "Commit state did not match %d %d\n" COLOR_NORMAL, confCommited, commitState);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Commit state was %d\n" COLOR_NORMAL, confCommited);
	}
}

// Tests that entire conf buffer is clear
void conf_is_empty() {
	if (ensureEmpty(0, CONFIGLEN)) {
		printf("" COLOR_GREEN "Config was empty\n" COLOR_NORMAL);
	}
}

// Fills conf buffer with CLEARBYTEs to allow knowing if a nullbyte has been written
void conf_clear() {
	memset(confBuffer, CLEARBYTE, CONFIGLEN);
	confCommited = 0;
}
