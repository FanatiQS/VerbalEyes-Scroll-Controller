#include <stdio.h> // printf, fprintf, fflush, stdout, EOF, stderr
#include <string.h> // strlen, memset, memcmp, strcmp
#include <time.h> // time_t, time, NULL
#include <stdbool.h> // bool, true, false

#include "../src/scroll_controller.h"
#include "./helpers/print_colors.h"

// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
int8_t verbaleyes_network_connected() { return 1; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
int8_t verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 1; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}


// All configurable strings lengths (copy from scroll_controller.c)
#define CONF_LEN_SSID           32
#define CONF_LEN_SSIDKEY        63
#define CONF_LEN_HOST           64
#define CONF_LEN_PATH           32
#define CONF_LEN_PROJ           32
#define CONF_LEN_PROJKEY        32

// All configurable items addesses (copy from scroll_controller.c)
#define CONF_ADDR_SSID          0
#define CONF_ADDR_SSIDKEY       (CONF_ADDR_SSID + CONF_LEN_SSID)
#define CONF_ADDR_HOST          (CONF_ADDR_SSIDKEY + CONF_LEN_SSIDKEY)
#define CONF_ADDR_PORT          (CONF_ADDR_HOST + CONF_LEN_HOST)
#define CONF_ADDR_PATH          (CONF_ADDR_PORT + 2)
#define CONF_ADDR_PROJ          (CONF_ADDR_PATH + CONF_LEN_PATH)
#define CONF_ADDR_PROJKEY       (CONF_ADDR_PROJ + CONF_LEN_PROJ)
#define CONF_ADDR_SPEEDMIN      (CONF_ADDR_PROJKEY + CONF_LEN_PROJKEY)
#define CONF_ADDR_SPEEDMAX      (CONF_ADDR_SPEEDMIN + 2)
#define CONF_ADDR_DEADZONE      (CONF_ADDR_SPEEDMAX + 2)
#define CONF_ADDR_CALLOW        (CONF_ADDR_DEADZONE + 2)
#define CONF_ADDR_CALHIGH       (CONF_ADDR_CALLOW + 2)
#define CONF_ADDR_SENS          (CONF_ADDR_CALHIGH + 2)

// Counter for the number of errors that occurs
int numberOfErrors = 0;




// Log buffer and index
size_t logBufferIndex = 0;
char logBuffer[1024];

// Indicates if logs should be buffered and/or printed
bool disableLogBuffering = false;
bool disableLogPrinting = true;

// Clears log buffer
void clearLogBuffer() {
	memset(logBuffer, 0, sizeof(logBuffer));
	logBufferIndex = 0;
}

// Compares log buffer to string
// @todo make logging index and data of config look better
void compareLogToString(const char* str) {
	// Prints success
	if (!strcmp(logBuffer, str)) {
		printf("" COLOR_GREEN "Logs matched\n" COLOR_NORMAL);
		return;
	}

	// Prints failure
	int len = (strlen(str) < strlen(logBuffer)) ? strlen(str) : strlen(logBuffer);
	fprintf(stderr, "" COLOR_RED "Logs did not match:\n|>%s<|\n|>%s<|\n" COLOR_NORMAL, logBuffer, str);
	fprintf(stderr, "" COLOR_RED "Lengths: %lu %lu\n" COLOR_NORMAL, strlen(str), strlen(logBuffer));
	for (int i = 0; i < len; i++) {
		fprintf(stderr, "index: %d  %d  %d\n", i, logBuffer[i], str[i]);
	}
	fprintf(stderr, "\n");
	numberOfErrors++;
}

// Logs VerbalEyes data to console and/or buffer
void verbaleyes_log(const char* msg, const size_t len) {
	// Ensures that log message is the expected length
	if (strlen(msg) != len) {
		fprintf(stderr, "" COLOR_RED "\nMessage to be logged was not the same length as its len argument\n\tstrlen(msg): %zu\n\tlen: %zu\n\tmessage: %s\n" COLOR_NORMAL, strlen(msg), len, msg);
		numberOfErrors++;
		exit(EXIT_FAILURE);
	}

	// Prints log to stdout
	if (!disableLogPrinting) {
		printf("%s", msg);
		fflush(stdout);
	}

	// Copies log message to log buffer
	if (!disableLogBuffering) {
		if (logBufferIndex + strlen(msg) > sizeof(logBuffer)) {
			perror("" COLOR_RED "Too much data logged without clearing\n" COLOR_NORMAL);
			exit(EXIT_FAILURE);
		}
		memcpy(logBuffer + logBufferIndex, msg, len);
		logBufferIndex += len;
	}
}





// Automatically exits configuration mode after test
bool allowOverwritingConfigData = true;

// Byte indicating empty data, not using null byte to separate from null terminated string
#define CLEARBYTE (3)

// Buffer for configuration
char configBuffer[VERBALEYES_CONFIGLEN];

// Commit state for configuration
bool configCommit = false;

// Fills config buffer with CLEARBYTEs to separate from nullbytes written
void clearConfigBuffer() {
	memset(configBuffer, CLEARBYTE, VERBALEYES_CONFIGLEN);
	configCommit = false;
}

// Checks if config buffer is empty or has data
// @todo make logging index and data of config look a little better
bool configIsEmpty(int index, const int len) {
	bool gotError = false;
	while (index < len) {
		if (configBuffer[index] != CLEARBYTE) {
			if (!gotError) {
				fprintf(stderr, "" COLOR_RED "Configuration was updated at an address where it should not have been:\n" COLOR_NORMAL);
				numberOfErrors++;
			}
			fprintf(stderr, "index: %d \tcode: %d \tchar: %c \n", index, configBuffer[index], configBuffer[index]);
			gotError = true;
		}
		index++;
	}
	return !gotError;
}

// Compares config to another buffer
// @todo make dumping entire config look a little better
void compareConfigToBuffer(const int offset, const char* buffer, const int len) {
	// Prints errors if config and buffer is not equal
	if (memcmp(configBuffer + offset, buffer, len)) {
		fprintf(stderr, "" COLOR_RED "Configuration did not match buffer:\n" COLOR_NORMAL);
		numberOfErrors++;
		for (int i = 0; i < len; i++) {
			fprintf(stderr,
				"index: %d  %u  %u\n",
				i + offset,
				(unsigned char)configBuffer[i + offset],
				(unsigned char)buffer[i]
			);
		}
	}
	// Prints message if config and buffer is equal
	else {
		printf("" COLOR_GREEN "Configuration buffer matched\n" COLOR_NORMAL);
	}

	// Ensures no data was written to addresses outside range of last write
	if (
		!allowOverwritingConfigData &&
		configIsEmpty(0, offset) &&
		configIsEmpty(offset + len, VERBALEYES_CONFIGLEN)
	) {
		printf("" COLOR_GREEN "Configuration buffer was only updated within its range\n" COLOR_NORMAL);
	}
}

// Compares config buffer to int
// @todo rework this function to instead actually work on uint16
void compareConfigToInteger(const int offset, const char* conf) {
	compareConfigToBuffer(offset, conf, 2);
}

// Compares config commit states
void compareConfigCommitState(const bool commitState) {
	if (configCommit != commitState) {
		fprintf(stderr, "" COLOR_RED "Commit state did not match %d %d\n" COLOR_NORMAL, configCommit, commitState);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Commit state was %d\n" COLOR_NORMAL, configCommit);
	}
}

// Updates configuration with string
void configureString(const char* str) {
	for (int i = 0; i < strlen(str); i++) {
		verbaleyes_configure(str[i]);
	}
}

// Updates configuration with a single character
void configureChar(const char c) {
	verbaleyes_configure(c);
}

// Exits configuration mode
void configureExit() {
	configureString("\n\n");
	configureChar('\0');
	clearLogBuffer();
}





// Prints test header
void testStart(const char* category, const char* title) {
	printf("" COLOR_BLUE "Testing %s: %s\n" COLOR_NORMAL, category, title);
	fflush(stdout);
}

// Prints test end message
void testEnd() {
	printf("" COLOR_GREEN "Test complete\n\n" COLOR_NORMAL);
	fflush(stdout);
}





// Check states for conf and log
#define ENDNOTHING 0
#define ENDDONE 1
#define ENDCOMMIT 2

// VerbalEyes function to write data to conf buffer
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	// Ensures address is not outside max config length
	if (addr >= VERBALEYES_CONFIGLEN) {
		fprintf(stderr, "" COLOR_RED "Configuration tried to write outide address range: %d\n" COLOR_NORMAL, addr);
		exit(EXIT_FAILURE);
	}

	// Disallows overwriting data in config
	if (!allowOverwritingConfigData && configBuffer[addr] != CLEARBYTE) {
		fprintf(stderr, "" COLOR_RED "Attempt to overwrite conf addr that has already been written to: %d\n" COLOR_NORMAL, addr);
		numberOfErrors++;
	}
	configBuffer[addr] = c;
}

// VerbalEyes function to commit data to conf buffer
void verbaleyes_conf_commit() {
	configCommit = true;
}

// VerbalEyes function to read data from conf buffer
char verbaleyes_conf_read(const unsigned short addr) {

	if (addr >= VERBALEYES_CONFIGLEN) {
		fprintf(stderr, "" COLOR_RED "Configuration tried to read outside address range: %d\n" COLOR_NORMAL, addr);
		exit(EXIT_FAILURE);
	}

	return configBuffer[addr];
}

// Starts a new config test
void configTestStart(const char* title, const char* input, const char* log1, const char* log2) {
	// Prints test start message
	testStart("configuration", title);

	// Clears buffers before running test
	clearLogBuffer();
	if (!allowOverwritingConfigData) clearConfigBuffer();

	// Processes input data
	for (int i = 0; input[i] != '\n'; i++) {
		if (verbaleyes_configure(input[i])) continue;
		fprintf(stderr, "" COLOR_RED "Updating configuration exited too early\n" COLOR_NORMAL);
		numberOfErrors++;
	}

	// Skips CRLF at start of log for comparison when auto resetting between tests
	if (allowOverwritingConfigData && strlen(log1) != 0) log1 += 2;

	// Compares log buffer to comparison string
	compareLogToString(log1);

	// Commits input and compares log message to log2
	clearLogBuffer();
	configureChar('\n');
	compareLogToString(log2);
}

// Ends running config test
void configTestEnd(const int state) {
	// Ensures that config did not commit its data yet
	compareConfigCommitState(false);

	// Only exits configuration mode during second pass
	if (allowOverwritingConfigData) {
		printf("" COLOR_GREEN "Not exiting config mode\n" COLOR_NORMAL);
		testEnd();
		return;
	}

	// Clears log buffer before exiting configuration mode
	clearLogBuffer();

	// Exits configuration mode
	configureChar('\n');
	if (verbaleyes_configure('\0')) {
		fprintf(stderr, "" COLOR_RED "Did not exit configuration mode after LF\n" COLOR_NORMAL);
		numberOfErrors++;
	}

	// Ensures that done message was printed
	if (state == ENDCOMMIT) {
		compareLogToString("Configuration saved\r\n");
	}
	// Ensures that cancel message was printed
	else if (state == ENDDONE) {
		compareLogToString("Configuration canceled\r\n");
	}
	// Ensures that no message was printed
	else {
		compareLogToString("");
	}

	// Ensures that config did or did not commit its data
	compareConfigCommitState(state == ENDCOMMIT);

	// Prints end of test message
	printf("" COLOR_GREEN "Exiting config mode\n" COLOR_NORMAL);
	testEnd();
}

// Runs test for an input that results in an empty config
void configTestEmpty(
	const char* title,
	const char* input,
	const char* log1,
	const char* log2,
	const int endState
) {
	configTestStart(title, input, log1, log2);
	if (!allowOverwritingConfigData && configIsEmpty(0, VERBALEYES_CONFIGLEN)) {
		printf("" COLOR_GREEN "Config was empty\n" COLOR_NORMAL);
	}
	configTestEnd(endState);
}

// Runs test for an input that results in an string written to the config
void configTestString(
	const char* title,
	const char* input,
	const char* log1,
	const char* log2,
	const int offset,
	const char* value,
	const bool incTerm
) {
	configTestStart(title, input, log1, log2);
	compareConfigToBuffer(offset, value, strlen(value) + incTerm);
	configTestEnd(ENDCOMMIT);
}

// Runs test for an input that results in an integer written to the config
void configTestInteger(
	const char* title,
	const char* input,
	const char* log1,
	const char* log2,
	const int offset,
	const char* value
) {
	configTestStart(title, input, log1, log2);
	compareConfigToInteger(offset, value);
	configTestEnd(ENDCOMMIT);
}

// Tests configuration input combinations
void configTestInputs() {
	// Tests that a comment will not print or commit conf changes
	configTestEmpty(
		"Comment",
		"# comment \0\0\0\0= do nothing\n",
		"",
		"",
		ENDNOTHING
	);

	// Tests that a valid key updates its value correctly
	configTestString(
		"Key valid",
		"ssid=123=\0\0\0\0=456\n",
		"\r\n[ ssid ] is now: 123==456",
		"\r\n",
		CONF_ADDR_SSID,
		"123==456",
		true
	);

	// Tests that an invalid key does not update anything
	configTestEmpty(
		"Key invalid",
		"ssif=123==456\n",
		"\r\n[ ssif ] No matching key",
		"\r\n",
		ENDDONE
	);

	// Tests that empty key does not update anything
	configTestEmpty(
		"Key empty",
		"=123==456\n",
		"\r\n[ ] No matching key",
		"\r\n",
		ENDDONE
	);

	// Tests that aborting key does not update anything
	configTestEmpty(
		"Key abortion",
		"ssid\n",
		"\r\n[ ssid",
		" ] Aborted\r\n",
		ENDDONE
	);

	// Tests that an empty value is updated correctly
	configTestString(
		"Value string empty",
		"ssid=\n",
		"\r\n[ ssid ] is now: ",
		"\r\n",
		CONF_ADDR_SSID,
		"",
		true
	);

	// Tests that a max length string updates without terminator
	configTestString(
		"Value string max length",
		"ssid=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
		"\r\n[ ssid ] is now: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		"\r\n",
		CONF_ADDR_SSID,
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		false
	);

	// Tests that over max length string only updates up to max length without terminator
	configTestString(
		"Value string too long",
		"ssid=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbb\n",
		"\r\n[ ssid ] is now: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\nMaximum input length reached",
		"\r\n",
		CONF_ADDR_SSID,
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		false
	);

	// Tests that non-alphanumeric characters for int types aborts
	configTestInteger(
		"Number with letters",
		"port=257abc123\n",
		"\r\n[ port ] is now: 257\r\nInvalid input (a)",
		"\r\n",
		CONF_ADDR_PORT,
		"\x01\x01"
	);

	// Tests that non-alphanumeric characters for int types aborts
	configTestInteger(
		"Number with only letters",
		"port=abc\n",
		"\r\n[ port ] is now: 0\r\nInvalid input (a)",
		"\r\n",
		CONF_ADDR_PORT,
		"\x00\x00"
	);

	// Tests that max unsigned integer updates correctly
	configTestInteger(
		"Number max uint",
		"port=65535\n",
		"\r\n[ port ] is now: 65535",
		"\r\n",
		CONF_ADDR_PORT,
		"\xff\xff"
	);

	// Tests that small unsigned integer updates correctly
	configTestInteger(
		"Number small uint",
		"port=128\n",
		"\r\n[ port ] is now: 128",
		"\r\n",
		CONF_ADDR_PORT,
		"\x00\x80"
	);

	// Tests that above max unsigned integer updates correctly
	configTestInteger(
		"Number above max uint",
		"port=65536\n",
		"\r\n[ port ] is now: 65536\r\nValue was too high and clamped down to maximum value of 65535",
		"\r\n",
		CONF_ADDR_PORT,
		"\xff\xff"
	);

	// Tests that too many characters for unsigned integer updates correctly
	configTestInteger(
		"Number max uint too long",
		"port=1111111111\n",
		"\r\n[ port ] is now: 111111\r\nValue was too high and clamped down to maximum value of 65535",
		"\r\n",
		CONF_ADDR_PORT,
		"\xff\xff"
	);

	// Tests that max signed integer updates correctly
	configTestInteger(
		"Number max int",
		"speedmin=32767\n",
		"\r\n[ speedmin ] is now: 32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x7f\xff"
	);

	// Tests that above max signed integer updates correctly
	configTestInteger(
		"Number above max int",
		"speedmin=32768\n",
		"\r\n[ speedmin ] is now: 32768\r\nValue was too high and clamped down to maximum value of 32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x7f\xff"
	);

	// Tests that too many characters for positive signed integer updates correctly
	configTestInteger(
		"Number max positive int too long",
		"speedmin=1111111111\n",
		"\r\n[ speedmin ] is now: 111111\r\nValue was too high and clamped down to maximum value of 32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x7f\xff"
	);

	// Tests that min signed integer updates correctly
	configTestInteger(
		"Number min int",
		"speedmin=-32767\n",
		"\r\n[ speedmin ] is now: -32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x80\x01"
	);

	// Tests that below min signed integer updates correctly
	configTestInteger(
		"Number below min int",
		"speedmin=-32768\n",
		"\r\n[ speedmin ] is now: -32768\r\nValue was too low and clamped up to minimum value of -32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x80\x01"
	);

	// Tests that too many characters for negative signed integer updates correctly
	configTestInteger(
		"Number min negative int too long",
		"speedmin=-1111111111\n",
		"\r\n[ speedmin ] is now: -111111\r\nValue was too low and clamped up to minimum value of -32767",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x80\x01"
	);

	// Tests that negative input for unsigned integer updates correctly
	configTestInteger(
		"Number negative for uint",
		"port=-32767\n",
		"\r\n[ port ] is now: 0\r\nInvalid input (-)",
		"\r\n",
		CONF_ADDR_PORT,
		"\x00\x00"
	);

	// Tests that a minus sign in the middle of input updates correctly
	configTestInteger(
		"Number misplaced minus sign",
		"speedmin=32-767\n",
		"\r\n[ speedmin ] is now: 32\r\nInvalid input (-)",
		"\r\n",
		CONF_ADDR_SPEEDMIN,
		"\x00\x20"
	);

	// Tests that ignored characters returns the correct value
	testStart("configuration", "Ignored characters");
	if (verbaleyes_configure('\r') != allowOverwritingConfigData) {
		fprintf(stderr, "" COLOR_RED "Ignored characters did not return the expected value\n" COLOR_NORMAL);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Ignored character returned the expected value\n" COLOR_NORMAL);
	}
	testEnd();

	// Tests if macro CONFIGLEN is correct length
	// @todo this does not really test configlen macro
	configTestStart("Config Macro", "sensitivity=65535\n", "\r\n[ sensitivity ] is now: 65535", "\r\n");
	compareConfigToInteger(CONF_ADDR_SENS, "\xff\xff");
	configTestEnd(ENDCOMMIT);

	// Tests that no data for a specified timeout would automatically exit configuration mode
	configTestStart("Timeout aborted", "1\n", "\r\n[ 1", " ] Aborted\r\n");
	fflush(stdout);
	time_t t1 = time(NULL);
	clearLogBuffer();
	while (verbaleyes_configure(0));
	if (!allowOverwritingConfigData) {
		compareLogToString("Configuration canceled\r\n");
	}
	else {
		compareLogToString("Configuration saved\r\n");
	}
	time_t t2 = time(NULL);
	if (t2 - t1 < 2) {
		fprintf(stderr, "" COLOR_RED "Timeout did not delay\n\n" COLOR_NORMAL);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Timeout delayed correctly\n\n" COLOR_NORMAL);
	}
}

// Fills string configuration with 0 characters
void fillConfigString(const char* key) {
	configureString(key);
	configureChar('=');
	for (int i = 0; i < VERBALEYES_CONFIGLEN; i++) configureChar('0');
	configureChar('\n');
}

// Fills integer configuration with 0 characters
void fillConfigInteger(const char* key) {
	configureString(key);
	configureString("=12336\n");
}

// Ensures that string configurations only writes data where it should
void configAddrRangeString(const char* key, const int offset, const int len) {
	// Prints test start message
	testStart("address mapping", key);

	// Clears config buffer before running test
	clearConfigBuffer();

	// Fills config for the key to its limit
	fillConfigString(key);

	// Compares config buffer to expected buffer
	char buf[VERBALEYES_CONFIGLEN];
	memset(buf, '0', len);
	compareConfigToBuffer(offset, buf, len);

	// Ends test
	testEnd();
}

// Ensures that integer configurations only writes data where it should
void configAddrRangeInteger(const char* key, const int offset) {
	// Prints test start message
	testStart("address mapping", key);

	// Clears config buffer before running test
	clearConfigBuffer();

	// Fills config for the key to its limit
	fillConfigInteger(key);

	// Compares config buffer to expected buffer
	char buf[2];
	memset(buf, '0', 2);
	compareConfigToBuffer(offset, buf, 2);

	// Ends test
	testEnd();
}

// Writes short string to ensure old data is not corrupting
void ensureShortConfigString(const char* key, const int offset) {
	testStart("overwriting shorter", key);
	clearConfigBuffer();
	configureString(key);
	configureString("=abc\n\n");
	configureChar('\0');
	compareConfigToBuffer(offset, "abc", 4);
}

// Writes short integer to ensure old data is not corrupting
void ensureShortConfigInteger(const char* key, const int offset) {
	testStart("overwriting shorter", key);
	clearConfigBuffer();
	configureString(key);
	configureString("=100\n\n");
	configureChar('\0');
	compareConfigToBuffer(offset, "\0d", 2);
	testEnd();
}

// All tests related to configuration
void testConfiguration() {
	// Enables log buffering
	disableLogBuffering = false;

	// Clears config buffer before tests
	clearConfigBuffer();

	// Runs all tests not exiting configuration mode between every test
	printf("\n\n");
	configureExit();
	configureString("1\n");
	configTestInputs();

	// Runs all tests again exiting configuration mode between every test
	printf("\n\n");
	allowOverwritingConfigData = false;
	configureExit();
	configTestInputs();
	allowOverwritingConfigData = true;

	// Tests that EOF, 0 and LF all return false if it is not already processing something else
	testStart("configuration", "First character EOF, 0 or LF");
	configureExit();
	clearConfigBuffer();
	int ignoreChars[] = { EOF, 0, '\n' };
	for (int i = 0; i < sizeof(ignoreChars) / sizeof(ignoreChars[0]); i++) {
		if (verbaleyes_configure(ignoreChars[i])) {
			fprintf(stderr, "" COLOR_RED "Character 0x%x did not return false\n" COLOR_NORMAL, ignoreChars[i]);
			numberOfErrors++;
		}
		compareLogToString("");
	}
	testEnd();



	// Tests address mapping
	printf("\n\n");
	disableLogBuffering = true;

	// Ensures configuration does not write outside its designated address range
	configAddrRangeString("ssid", CONF_ADDR_SSID, CONF_LEN_SSID);
	configAddrRangeString("ssidkey", CONF_ADDR_SSIDKEY, CONF_LEN_SSIDKEY);
	configAddrRangeString("host", CONF_ADDR_HOST, CONF_LEN_HOST);
	configAddrRangeInteger("port", CONF_ADDR_PORT);
	configAddrRangeString("path", CONF_ADDR_PATH, CONF_LEN_PATH);
	configAddrRangeString("proj", CONF_ADDR_PROJ, CONF_LEN_PROJ);
	configAddrRangeString("projkey", CONF_ADDR_PROJKEY, CONF_LEN_PROJKEY);
	configAddrRangeInteger("speedmin", CONF_ADDR_SPEEDMIN);
	configAddrRangeInteger("speedmax", CONF_ADDR_SPEEDMAX);
	configAddrRangeInteger("deadzone", CONF_ADDR_DEADZONE);
	configAddrRangeInteger("callow", CONF_ADDR_CALLOW);
	configAddrRangeInteger("calhigh", CONF_ADDR_CALHIGH);
	configAddrRangeInteger("sensitivity", CONF_ADDR_SENS);

	// Checks configuration buffer for gaps
	printf("\n\n");
	testStart("address mapping", "Looking for gaps");
	configureExit();
	clearConfigBuffer();
	fillConfigString("ssid");
	fillConfigString("ssidkey");
	fillConfigString("host");
	fillConfigInteger("port");
	fillConfigString("path");
	fillConfigString("proj");
	fillConfigString("projkey");
	fillConfigInteger("speedmin");
	fillConfigInteger("speedmax");
	fillConfigInteger("deadzone");
	fillConfigInteger("callow");
	fillConfigInteger("calhigh");
	fillConfigInteger("sensitivity");
	int foundGaps = 0;
	for (int i = 0; i < VERBALEYES_CONFIGLEN; i++) {
		if (configBuffer[i] != '0') {
			fprintf(stderr, "" COLOR_RED "Gap was left at address: %d\n" COLOR_NORMAL, i);
			foundGaps++;
		}
	}
	if (foundGaps) {
		fprintf(stderr, "" COLOR_RED "%d gaps were found in conf buffer\n" COLOR_NORMAL, foundGaps);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Entire conf buffer was filled correctly\n" COLOR_NORMAL);
	}



	// Tests overwriting shorter messages
	printf("\n\n");
	configureExit();
	ensureShortConfigString("ssid", CONF_ADDR_SSID);
	ensureShortConfigString("ssidkey", CONF_ADDR_SSIDKEY);
	ensureShortConfigString("host", CONF_ADDR_HOST);
	ensureShortConfigInteger("port", CONF_ADDR_PORT);
	ensureShortConfigString("path", CONF_ADDR_PATH);
	ensureShortConfigString("proj", CONF_ADDR_PROJ);
	ensureShortConfigString("projkey", CONF_ADDR_PROJKEY);
	ensureShortConfigInteger("speedmin", CONF_ADDR_SPEEDMIN);
	ensureShortConfigInteger("speedmax", CONF_ADDR_SPEEDMAX);
	ensureShortConfigInteger("deadzone", CONF_ADDR_DEADZONE);
	ensureShortConfigInteger("callow", CONF_ADDR_CALLOW);
	ensureShortConfigInteger("calhigh", CONF_ADDR_CALHIGH);
	ensureShortConfigInteger("sensitivity", CONF_ADDR_SENS);
}



int main(void) {
	testConfiguration();

	// Prints the number of errors that occured
	printf("\n\n" COLOR_BLUE "RESULT:\n" COLOR_NORMAL);
	printf((numberOfErrors) ? COLOR_RED : COLOR_GREEN);
	printf("Number of errors: %d\n", numberOfErrors);
	printf("" COLOR_NORMAL);
	return (numberOfErrors) ? EXIT_FAILURE : EXIT_SUCCESS;
}
