#include <stdio.h> // printf, fprintf, stderr
#include <string.h> // strlen
#include <stdbool.h> // bool
#include <time.h> // clock_t

#include "../src/scroll_controller.h"

#include "./helpers/print_colors.h"
#include "./helpers/conf.h"
#include "./helpers/log.h"
#include "./helpers/debug.h"



// Use short conf values
#define SHORTCONFLEN 3
bool useShortConf = 0;

// Fills configuration to the max length
void fillConfChar(const char* key, char c) {
	for (int i = 0; i < strlen(key); i++) verbaleyes_configure(key[i]);
	verbaleyes_configure('=');
	for (int i = 0; i < ((useShortConf) ? SHORTCONFLEN : VERBALEYES_CONFIGLEN); i++) {
		verbaleyes_configure(c);
	}
	verbaleyes_configure('\n');
}

// Makes sure a string only contains the correct character
bool checkConfStr(const char* str, char c) {
	bool err = 0;
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] != c) {
			fprintf(stderr, "" COLOR_RED "Configuration contained incorrect data:\n\tindex: %d\n\tstring char: %d\n\tchar: %d\n" COLOR_NORMAL, i, str[i], c);
			err = 1;
		}
	}
	if (!err) {
		//!! printf("" COLOR_GREEN "Configuration item matched\n" COLOR_NORMAL);
	}
	return err;
}

// Indicates what test to run
int testState = 0;
bool testDroppedConnection = false;

// Runs a test
void testInit(const char* state, char* log) {
	// Prints test id
	printf("\nTest: %d\n", testState);

	// Clears log buffer
	log_clear();

	// Run initialize function until error or success
	while (verbaleyes_initialize() == 1);

	// Compare log buffer
	if (useShortConf) log_cmp(log);

	// Increments test state for next test
	testState++;
	testDroppedConnection = false;
}



// Tests length and value of ssid and ssidkey
void verbaleyes_network_connect(const char* ssid, const char* key) {
	static bool ssidLenErr = false;
	static bool ssidValErr = false;
	static bool ssidkeyLenErr = false;
	static bool ssidkeyValErr = false;

	// Tests length and value of ssid
	if (!ssidLenErr && strlen(ssid) != ((useShortConf) ? SHORTCONFLEN : 32)) {
		fprintf(stderr, "" COLOR_RED "Length of ssid was incorrect: %lu\n" COLOR_NORMAL, strlen(ssid));
		numberOfErrors++;
		ssidLenErr = true;
	}
	if (!ssidValErr && checkConfStr(ssid, 'a')) {
		numberOfErrors++;
		ssidValErr = true;
	}

	// Tests length and value of ssidkey
	if (!ssidkeyLenErr && strlen(key) != ((useShortConf) ? SHORTCONFLEN : 63)) {
		fprintf(stderr, "" COLOR_RED "Length of ssidkey was incorrect: %lu\n" COLOR_NORMAL, strlen(key));
		numberOfErrors++;
		ssidkeyLenErr = true;
	}
	if (!ssidkeyValErr && checkConfStr(key, 'b')) {
		numberOfErrors++;
		ssidkeyValErr = true;
	}
}

// Tests all states related to network connection
int8_t verbaleyes_network_connected() {
	switch (testState) {
		// Tests rejected network
		case 0: return -1;

		// Tests established network connection
		case 1: return 1;

		// Tests connection lost and awaiting network connection
		case 2: return 0;

		// Connection should be established for other tests to work
		default: return 1;
	}
}

// Tests length and value of host and port
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
	static bool hostLenErr = false;
	static bool hostValErr = false;
	static bool portErr = false;

	// Test length of host
	if (!hostLenErr && strlen(host) != ((useShortConf) ? SHORTCONFLEN : 64)) {
		fprintf(stderr, "" COLOR_RED "Length of host was incorrect: %lu\n" COLOR_NORMAL, strlen(host));
		numberOfErrors++;
		hostLenErr = true;
	}
	if (!hostValErr && checkConfStr(host, 'c')) {
		numberOfErrors++;
		hostValErr = true;
	}

	// Tests value of port
	if (!portErr && port != 1111) {
		fprintf(stderr, "" COLOR_RED "Value of port was incorrect: %d\n" COLOR_NORMAL, port);
		numberOfErrors++;
		portErr = true;
	}
}

// Tests all states related to socket connection
int8_t verbaleyes_socket_connected() {
	switch (testState) {
		// Tests rejected socket connection
		case 1: return -1;

		// Tests awaiting socket connection
		case 3: return 0;

		// Tests established socket connection
		case 4: return 1;

		// Tests socket disconnect
		case 5: {
			if (testDroppedConnection) return 0;
			testDroppedConnection = true;
			return 1;
		}

		// Tests connection lost
		case -1: return 0; //!! requires entire socket connection to be established first

		// Connection should be established for other tests
		default: return 1;
	}
}
// Reads socket data
int16_t verbaleyes_socket_read() {
	switch (testState) {
		// Has no data
		default: return EOF;
	}
}

// WebSocket request strings for writing or comparing reads
#define WS_REQ "GET ddd HTTP/1.1\r\nHost: ccc\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: "
#define WS_KEY "nxZqCIY+DNYVMizHXRYUvQ=="

// Forces same random seed to be used every time
clock_t clock() { return 1; }

// Writes socket data
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {
	switch (testState) {
		// Tests WebSocket request / path
		case 5: {
			// Tests content of WebSocket request
			if (useShortConf) {
				if (strcmp((char*)str, WS_REQ WS_KEY "\r\n\r\n")) {
					fprintf(stderr, "" COLOR_RED "HTTP request does not look correct\n" COLOR_NORMAL);
					numberOfErrors++;
				}
				else {
					printf("" COLOR_GREEN "HTTP request looks correct\n" COLOR_NORMAL);
				}
			}
			// Tests length of path
			else {
				const char* path = (char*)str + 4;
				int len = 0;
				while (path[len] == 'd') len++;
				if (len != 32) {
					fprintf(stderr, "" COLOR_RED "Length of path was incorrect: %d\n" COLOR_NORMAL, len);
					numberOfErrors++;
				}
			}
		}
	}
}



// Log messages used for network
#define LOG_NETWORK_1 "\r\nConnecting to SSID: aaa..."
#define LOG_NETWORK_2 "\r\nFailed to connect to network"
#define LOG_NETWORK_3 "\r\nNetwork connection established"

// Log messaged used for socket
#define LOG_SOCKET_1 "\r\nConnecting to host: ccc:1111..."
#define LOG_SOCKET_2 "\r\nFailed to connect to host"
#define LOG_SOCKET_3 "\r\nAccessing WebSocket server at ddd..."
#define LOG_SOCKET_4 "\r\nDid not get a response from the server"
#define LOG_SOCKET_5 "\r\nConnection to host closed"

// Other log messages
#define LOG_PROGRESSBAR "..."


// Tests initilization function with long or short names
void runTests(bool useShort) {
	// Updates all config items
	useShortConf = useShort;
	fillConfChar("ssid", 'a');
	fillConfChar("ssidkey", 'b');
	fillConfChar("host", 'c');
	configure_str("port=1111\n");
	fillConfChar("path", 'd');
	fillConfChar("proj", 'e');
	fillConfChar("projkey", 'f');
	configure_str("speedmax=2222\n");
	configure_str("speedmin=3333\n");
	configure_str("deadzone=4444\n");
	configure_str("callow=5555\n");
	configure_str("calhigh=6666\n");
	configure_str("sensitivity=7777\n");
	verbaleyes_configure('\n');
	printf("\n\n");

	// Test network connection rejected
	testInit("", LOG_NETWORK_1 LOG_NETWORK_2);

	// Test network connection awaiting
	testInit("", LOG_NETWORK_1 LOG_PROGRESSBAR LOG_NETWORK_2);

	// Test network connection continue and socket connection rejected
	testInit("", LOG_NETWORK_1 LOG_NETWORK_3 LOG_SOCKET_1 LOG_SOCKET_2);

	// Test network connection lost and socket connection awaiting
	testInit("", "\r\nLost connection to network" LOG_NETWORK_1 LOG_NETWORK_3 LOG_SOCKET_1 LOG_PROGRESSBAR LOG_SOCKET_2);

	// Test network default continue and socket connection continue and ws response no data
	testInit("", LOG_SOCKET_1 LOG_SOCKET_3 LOG_PROGRESSBAR LOG_SOCKET_4);

	// Test ws response disconnect socket
	testInit("", LOG_SOCKET_1 LOG_SOCKET_3 LOG_SOCKET_5);
}



//!!
int main() {
	//!!
	log_setflags(LOGFLAGBUFFER | LOGFLAGPRINT);

	//!!
	runTests(1);

	//!!
	return debug_printerrors();
}



//!! test with max length conf and not max length
//!! add test in testInit for connection failed delay somehow


//!! in another file:
//!! write max length conf to all items, then run verbaleyes_initialize to check that entire length of configuration is read in.
//!! test that resestState is updated after successful configuration. Check that it goes back and reads the new data. So build a system that can extract the value from a call to verbaleyes_initialize.








//!!
char socket_read_data[] = "\0http/1.1 101 OK\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ICX+Yqv66kxgM0FcWaLWlFLwTAI=\r\n\r\n\0\x81\x09" "authed789\0";
int socket_read_index = 0;
short verbaleyes_socket_read2() {
	unsigned char c = socket_read_data[socket_read_index++];
	return (c == '\0') ? EOF : c;
 }
