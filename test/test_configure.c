#include <stdio.h> // printf, fprintf, fflush, stdout, EOF, stderr, stdouts
#include <string.h> // strlen
#include <time.h> // time_t, time, NULL

#include "../src/scroll_controller.h"

#include "./helpers/print_colors.h"
#include "./helpers/conf.h"
#include "./helpers/log.h"

// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
int8_t verbaleyes_network_connected() { return 1; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
int8_t verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 1; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
char verbaleyes_conf_read(const unsigned short addr) { return 1; }



// Number of errors occuring
int numberOfErrors = 0;

// Automatically exits configuration mode after test
bool autoReset = 0;

// Starts a new test
void test_start(const char* title, const char* input, const char* log) {
	// Prints test start message
	printf("" COLOR_BLUE "Testing configuring: %s\n" COLOR_NORMAL, title);
	fflush(stdout);
	log_clear();

	// Clears conf buffer when testing separate conf mode runs, only during second pass
	if (autoReset) conf_clear();

	// Processes input data
	if (updateConfig(input)) {
		fprintf(stderr, "" COLOR_RED "Updating configuration exited too early\n" COLOR_NORMAL);
	}

	// Compates buffered log message to log
	log_cmp(log);
}

// Tests that conf buffer is clean
void test_conf_empty() {
	if (autoReset) conf_is_empty();
}

// Compares conf buffer to string
void test_conf_string(const int offset, const char* conf, const int incTerm) {
	conf_cmp(offset, conf, strlen(conf) + incTerm);
}

// Compares conf buffer to int
void test_conf_int(const int offset, const char* conf) {
	conf_cmp(offset, conf, 2);
}

// Check states for conf and log
#define ENDNOTHING 0
#define ENDDONE 1
#define ENDCOMMIT 3

// Ends test
void test_end(int state) {
	// Tests that conf did not commit its data yet
	conf_matchcommit(0);

	// Only exits configuration mode during second pass
	if (autoReset) {
		// Clears log buffer before exiting configuration mode
		log_clear();

		// Exits configuration mode
		if (verbaleyes_configure('\n')) {
			fprintf(stderr, "" COLOR_RED "Did not exit configuration mode after LF\n" COLOR_NORMAL);
		}

		// Tests that "Done" message was printed
		if (state) {
			log_cmp("\r\nDone\r\n");
		}
		// Tests that no "Done" message was printed
		else {
			log_cmp("");
		}

		// Tests that conf did or did not commit its data
		conf_matchcommit(state & 0x02);
	}
	else {
		printf("" COLOR_GREEN "Not exiting config mode\n" COLOR_NORMAL);
	}
	printf("" COLOR_GREEN "Test successful\n\n" COLOR_NORMAL);
}



// Addresses of configuration items
#define ADDR_SSID 0
#define ADDR_PORT 159
#define ADDR_SPEEDMIN 257

// Tests configuration input combinations
void runTests() {
	// Tests that a comment will not print or commit conf changes
	test_start("Comment", "# comment \0\0\0\0= do nothing\n", "");
	test_conf_empty();
	test_end(ENDNOTHING);

	// Tests that a valid key updates its value correctly
	test_start("Key valid", "ssid=123=\0\0\0\0=456\n", "\r\n[ ssid ] is now: 123==456");
	test_conf_string(ADDR_SSID, "123==456", 1);
	test_end(ENDCOMMIT);

	// Tests that an invalid key does not update anything
	test_start("Key invalid", "ssif=123==456\n", "\r\n[ ssif ] No matching key");
	test_conf_empty();
	test_end(ENDDONE);

	// Tests that empty key does not update anything
	test_start("Key empty", "=123==456\n", "\r\n[ ] No matching key");
	test_conf_empty();
	test_end(ENDDONE);

	// Tests that aborting key does not update anything
	test_start("Key abortion", "ssid\n", "\r\n[ ssid ] Aborted");
	test_conf_empty();
	test_end(ENDDONE);

	// Tests that an empty value is updated correctly
	test_start("Value string empty", "ssid=\n", "\r\n[ ssid ] is now: ");
	test_conf_string(ADDR_SSID, "", 1);
	test_end(ENDCOMMIT);

	// Tests that a max length string updates without terminator
	test_start("Value string max length", "ssid=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n", "\r\n[ ssid ] is now: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	test_conf_string(ADDR_SSID, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0);
	test_end(ENDCOMMIT);

	// Tests that over max length string only updates up to max length without terminator
	test_start("Value string too long", "ssid=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbb\n", "\r\n[ ssid ] is now: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\nMaximum input length reached");
	test_conf_string(ADDR_SSID, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0);
	test_end(ENDCOMMIT);

	// Tests that non-alphanumeric characters for int types aborts
	test_start("Number with letters", "port=257abc123\n", "\r\n[ port ] is now: 257\r\nInvalid input (a)");
	test_conf_int(ADDR_PORT, "\x01\x01");
	test_end(ENDCOMMIT);

	// Tests that non-alphanumeric characters for int types aborts
	test_start("Number with only letters", "port=abc\n", "\r\n[ port ] is now: 0\r\nInvalid input (a)");
	test_conf_int(ADDR_PORT, "\x00\x00");
	test_end(ENDCOMMIT);

	// Tests that max unsigned integer updates correctly
	test_start("Number max uint", "port=65535\n", "\r\n[ port ] is now: 65535");
	test_conf_int(ADDR_PORT, "\xff\xff");
	test_end(ENDCOMMIT);

	// Tests that small unsigned integer updates correctly
	test_start("Number small uint", "port=128\n", "\r\n[ port ] is now: 128");
	test_conf_int(ADDR_PORT, "\x00\x80");
	test_end(ENDCOMMIT);

	// Tests that above max unsigned integer updates correctly
	test_start("Number above max uint", "port=65536\n", "\r\n[ port ] is now: 65536\r\nValue was too high and clamped down to maximum value of 65535");
	test_conf_int(ADDR_PORT, "\xff\xff");
	test_end(ENDCOMMIT);

	// Tests that too many characters for unsigned integer updates correctly
	test_start("Number max uint too long", "port=1111111111\n", "\r\n[ port ] is now: 111111\r\nValue was too high and clamped down to maximum value of 65535");
	test_conf_int(ADDR_PORT, "\xff\xff");
	test_end(ENDCOMMIT);

	// Tests that max signed integer updates correctly
	test_start("Number max int", "speedmin=32767\n", "\r\n[ speedmin ] is now: 32767");
	test_conf_int(ADDR_SPEEDMIN, "\x7f\xff");
	test_end(ENDCOMMIT);

	// Tests that above max signed integer updates correctly
	test_start("Number above max int", "speedmin=32768\n", "\r\n[ speedmin ] is now: 32768\r\nValue was too high and clamped down to maximum value of 32767");
	test_conf_int(ADDR_SPEEDMIN, "\x7f\xff");
	test_end(ENDCOMMIT);

	// Tests that too many characters for positive signed integer updates correctly
	test_start("Number max positive int too long", "speedmin=1111111111\n", "\r\n[ speedmin ] is now: 111111\r\nValue was too high and clamped down to maximum value of 32767");
	test_conf_int(ADDR_SPEEDMIN, "\x7f\xff");
	test_end(ENDCOMMIT);

	// Tests that min signed integer updates correctly
	test_start("Number min int", "speedmin=-32767\n", "\r\n[ speedmin ] is now: -32767");
	test_conf_int(ADDR_SPEEDMIN, "\x80\x01");
	test_end(ENDCOMMIT);

	// Tests that below min signed integer updates correctly
	test_start("Number below min int", "speedmin=-32768\n", "\r\n[ speedmin ] is now: -32768\r\nValue was too low and clamped up to minimum value of -32767");
	test_conf_int(ADDR_SPEEDMIN, "\x80\x01");
	test_end(ENDCOMMIT);

	// Tests that too many characters for negative signed integer updates correctly
	test_start("Number min negative int too long", "speedmin=-1111111111\n", "\r\n[ speedmin ] is now: -111111\r\nValue was too low and clamped up to minimum value of -32767");
	test_conf_int(ADDR_SPEEDMIN, "\x80\x01");
	test_end(ENDCOMMIT);

	// Tests that negative input for unsigned integer updates correctly
	test_start("Number negative for uint", "port=-32767\n", "\r\n[ port ] is now: 0\r\nInvalid input (-)");
	test_conf_int(ADDR_PORT, "\x00\x00");
	test_end(ENDCOMMIT);

	// Tests that a minus sign in the middle of input updates correctly
	test_start("Number misplaced minus sign", "speedmin=32-767\n", "\r\n[ speedmin ] is now: 32\r\nInvalid input (-)");
	test_conf_int(ADDR_SPEEDMIN, "\x00\x20");
	test_end(ENDCOMMIT);

	// Tests that ignored characters returns the correct value
	test_start("Ignored characters", "", "");
	if (verbaleyes_configure('\r') == autoReset) {
		fprintf(stderr, "" COLOR_RED "Ignored characters did not return false\n" COLOR_NORMAL);
		numberOfErrors++;
	}
	test_end(ENDNOTHING);

	// Tests if macro CONFIGLEN is correct length
	test_start("Config Macro", "sensitivity=65535\n", "\r\n[ sensitivity ] is now: 65535");
	test_conf_int(267, "\xff\xff");
	test_end(ENDCOMMIT);
}



// Writes max configuration
void fillConf(char* key, bool isStr) {
	int i;

	// Processes key and delimiter
	for (i = 0; i < strlen(key); i++) verbaleyes_configure(key[i]);
	verbaleyes_configure('=');

	// Processes string value
	if (isStr) {
		for (i = 0; i < CONFIGLEN; i++) verbaleyes_configure('0');
	}
	// Processes integer value
	else {
		char buf[] = "12336";
		for (i = 0; buf[i] != '\0'; i++) verbaleyes_configure(buf[i]);
	}
}

// Tests that every configuration only writes data where it should
void testConfAddr(char* key, int len, int offset) {
	// Prints test start message
	printf("" COLOR_BLUE "Testing configuration address: %s\n" COLOR_NORMAL, key);

	// Clears conf buffer before writing new data
	conf_clear();

	// Writes max length configuration
	fillConf(key, len > 0);

	// Exits configuration mode
	verbaleyes_configure('\n');

	// Compares conf buffer
	char buf[CONFIGLEN];
	memset(buf, '0', (len) ? len : 2);
	conf_cmp(offset, buf, (len) ? len : 2);
	printf("\n");
}



//!!
int main(void) {
	// Setup
	printf("\n\n");
	log_setflags(LOGFLAGBUFFER);
	conf_clear();
	//!! could get only errors by closing stdout

	// Run all tests
	runTests();

	// Run tests again exiting configuration mode between every tests
	printf("\n\n");
	autoReset = 1;
	conf_setflags(CONFFLAGCOMMITED | CONFFLAGOUTSIDESTR);
	verbaleyes_configure('\n');
	runTests();

	// Tests that EOF, 0 and LF all return false if it is not already processing something else
	test_start("First character EOF, 0 or LF", "", "");
	int ignoreChars[] = { EOF, 0, '\n' };
	for (int i = 0; i < sizeof(ignoreChars) / sizeof(ignoreChars[0]); i++) {
		if (verbaleyes_configure(ignoreChars[i])) {
			fprintf(stderr, "" COLOR_RED "Character written was %d and did not return false\n" COLOR_NORMAL, ignoreChars[i]);
			numberOfErrors++;
		}
		log_cmp("");
	}
	test_end(ENDNOTHING);

	// Tests that no data for a specified timeout would automatically exit configuration mode
	test_start("Timeout aborted", "1\n", "\r\n[ 1 ] Aborted");
	time_t t1 = time(NULL);
	log_clear();
	while (verbaleyes_configure(0));
	log_cmp("\r\nDone\r\n");
	time_t t2 = time(NULL);
	if (t2 - t1 < 2) {
		fprintf(stderr, "" COLOR_RED "Timeout did not delay\n\n" COLOR_NORMAL);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Timeout delayed correctly\n\n" COLOR_NORMAL);
	}

	// Tests configuration item addresses
	printf("\n\n");
	log_setflags(0);
	conf_setflags(CONFFLAGOUTSIDESTR);
	testConfAddr("ssid", 32, 0);
	testConfAddr("ssidkey", 63, 32);
	testConfAddr("host", 64, 32 + 63);
	testConfAddr("port", 0, 32 + 63 + 64);
	testConfAddr("path", 32, 32 + 63 + 64 + 2);
	testConfAddr("proj", 32, 32 + 63 + 64 + 2 + 32);
	testConfAddr("projkey", 32, 32 + 63 + 64 + 2 + 32 + 32);
	testConfAddr("speedmin", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32);
	testConfAddr("speedmax", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32 + 2);
	testConfAddr("deadzone", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2);
	testConfAddr("callow", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2);
	testConfAddr("calhigh", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2 + 2);
	testConfAddr("sensitivity", 0, 32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2 + 2 + 2);

	// Tests configuration buffer for gaps
	printf("\n\n");
	printf("" COLOR_BLUE "Testing configuration address: Looking for gaps\n" COLOR_NORMAL);
	conf_clear();
	fillConf("ssid", 1);
	verbaleyes_configure('\n');
	fillConf("ssidkey", 1);
	verbaleyes_configure('\n');
	fillConf("host", 1);
	verbaleyes_configure('\n');
	fillConf("port", 0);
	verbaleyes_configure('\n');
	fillConf("path", 1);
	verbaleyes_configure('\n');
	fillConf("proj", 1);
	verbaleyes_configure('\n');
	fillConf("projkey", 1);
	verbaleyes_configure('\n');
	fillConf("speedmin", 0);
	verbaleyes_configure('\n');
	fillConf("speedmax", 0);
	verbaleyes_configure('\n');
	fillConf("deadzone", 0);
	verbaleyes_configure('\n');
	fillConf("callow", 0);
	verbaleyes_configure('\n');
	fillConf("calhigh", 0);
	verbaleyes_configure('\n');
	fillConf("sensitivity", 0);
	verbaleyes_configure('\n');
	bool foundGaps = 0;
	for (int i = 0; i < CONFIGLEN; i++) {
		if (confBuffer[i] != '0') {
			fprintf(stderr, "" COLOR_RED "Gap was left at address: %d\n" COLOR_NORMAL, i);
			foundGaps = 1;
		}
	}
	if (foundGaps) {
		fprintf(stderr, "" COLOR_RED "Gaps were found in conf buffer\n" COLOR_NORMAL);
		numberOfErrors++;
	}
	else {
		printf("" COLOR_GREEN "Entire conf buffer was filled correctly\n" COLOR_NORMAL);
	}

	// Tests overwriting shorter messages
	printf("\n\n");
	printf("" COLOR_BLUE "Testing configuration address: Overwriting shorter\n" COLOR_NORMAL);
	conf_setflags(0);
	updateConfig("ssid=abc\n");
	conf_cmp(0, "abc", 4);
	updateConfig("ssidkey=abc\n");
	conf_cmp(32, "abc", 4);
	updateConfig("host=abc\n");
	conf_cmp(32 + 63, "abc", 4);
	updateConfig("port=100\n");
	conf_cmp(32 + 63 + 64, "\0d", 2);
	updateConfig("path=abc\n");
	conf_cmp(32 + 63 + 64 + 2, "abc", 4);
	updateConfig("proj=abc\n");
	conf_cmp(32 + 63 + 64 + 2 + 32, "abc", 4);
	updateConfig("projkey=abc\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32, "abc", 4);
	updateConfig("speedmin=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32, "\0d", 2);
	updateConfig("speedmax=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32 + 2, "\0d", 2);
	updateConfig("deadzone=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2, "\0d", 2);
	updateConfig("callow=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2, "\0d", 2);
	updateConfig("calhigh=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2 + 2, "\0d", 2);
	updateConfig("sensitivity=100\n");
	conf_cmp(32 + 63 + 64 + 2 + 32 + 32 + 32 + 2 + 2 + 2 + 2 + 2, "\0d", 2);

	//!!
	printf("\n\n");
	return 0;
}
