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
			numberOfErrors++;
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
int testReadIndex = 0;



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
		case 0: return VERBALEYES_CONNECT_FAIL;

		// Tests established network connection
		case 1: return VERBALEYES_CONNECT_SUCCESS;

		// Tests connection lost and awaiting network connection
		case 2: return VERBALEYES_CONNECT_WORKING;

		// Connection should be established for other tests to work
		default: return VERBALEYES_CONNECT_SUCCESS;
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
		case 1: return VERBALEYES_CONNECT_FAIL;

		// Tests awaiting socket connection
		case 3: return VERBALEYES_CONNECT_WORKING;

		// Tests established socket connection
		case 4: return VERBALEYES_CONNECT_SUCCESS;

		// Tests socket disconnect
		case 5: {
			if (testDroppedConnection) return VERBALEYES_CONNECT_WORKING;
			testDroppedConnection = true;
			return VERBALEYES_CONNECT_SUCCESS;
		}

		// Tests socket dropped during processing HTTP status line
		case 9: {
			if (testReadIndex > 0) return VERBALEYES_CONNECT_WORKING;
			return VERBALEYES_CONNECT_SUCCESS;
		}

		// Tests socket dropped during processing HTTP headers
		case 12: {
			if (testReadIndex > 0) return VERBALEYES_CONNECT_WORKING;
			return VERBALEYES_CONNECT_SUCCESS;
		}

		// Tests connection lost
		case -1: return VERBALEYES_CONNECT_WORKING; //!! requires entire socket connection to be established first

		// Connection should be established for other tests
		default: return VERBALEYES_CONNECT_SUCCESS;
	}
}



// EOF for string int8_t
#define EOFS "\xff"

// HTTP responses
#define HTTP_NOT "testing non-http response"
#define HTTP_NOT2 "http not"
#define HTTP_HALF "HTTP/1.1 10"
#define HTTP_404 "HTTP/1.1 404"
#define HTTP_STATUS "HTTP/1.1 101 Switching Protocols\r\n"
#define HTTP_HEADER_CONNECTION_WRONG "Connection: Keep-Alive\r\n"
#define HTTP_HEADER_CONNECTION "Connection: Upgrade\r\n"
#define HTTP_HEADER_UPGRADE_WRONG "Upgrade: HTTP/2.0\r\n"
#define HTTP_HEADER_UPGRADE "Upgrade: websocket\r\n"
#define HTTP_HEADER_KEY_WRONG "Sec-WebSocket-Accept: dGhlIHNhbXBsZSBub25jZQ==\r\n"
#define HTTP_HEADER_KEY "Sec-WebSocket-Accept: KM6afhWtO4bb/E9amIx6hTrgkCw=\r\n"
#define HTTP_HEADER_EXTENSION "Sec-WebSocket-Extensions: yes\r\n"
#define HTTP_HEADER_PROTOCOL "Sec-WebSocket-Protocol: yes\r\n"

// Gets next character of string cast to signed char for -1
int16_t getReadData(char* data) {
	if (testReadIndex > strlen(data) - 1) return EOF;
 	return (int8_t)data[testReadIndex++];
}

// Reads socket data
int16_t verbaleyes_socket_read() {
	switch (testState) {
		// Tests no response before timeout
		case 4: return EOF;

		// Tests no response before connection closed
		case 5: return EOF;

		// Tests non-HTTP response
		case 6: return getReadData(EOFS HTTP_NOT);

		// Tests non-HTTP response2
		case 7: return getReadData(EOFS HTTP_NOT2);

		// Tests half HTTP status line response
		case 8: return getReadData(EOFS HTTP_HALF);

		// Tests half HTTP status line and connection lost
		case 9: return getReadData(EOFS HTTP_HALF);

		// Tests invalid HTTP code
		case 10: return getReadData(EOFS HTTP_404);

		// Tests HTTP status line response and half headers
		case 11: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION);

		// Tests HTTP status line and half headers and connection lost
		case 12: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION);

		// Tests wrong connection type
		case 13: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION_WRONG "\r\n");

		// Tests wrong upgrade
		case 14: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION HTTP_HEADER_UPGRADE_WRONG "\r\n");

		// Tests wrong ws key
		case 15: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION HTTP_HEADER_UPGRADE HTTP_HEADER_KEY_WRONG "\r\n");

		// Tests present extension
		case 16: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION HTTP_HEADER_UPGRADE HTTP_HEADER_KEY HTTP_HEADER_EXTENSION "\r\n");

		// Tests present protocol
		case 17: return getReadData(EOFS HTTP_STATUS HTTP_HEADER_CONNECTION HTTP_HEADER_UPGRADE HTTP_HEADER_KEY HTTP_HEADER_PROTOCOL "\r\n");

		// There should be a case for every test calling this function
		default: {
			fprintf(stderr, "" COLOR_RED "Called verbaleyes_socket_read with unknown test: %d\n" COLOR_NORMAL, testState);
			exit(EXIT_FAILURE);
		}
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
#define LOG_SOCKET_6 "\r\nResponse from server ended prematurely"

// Log messages used for HTTP header
#define LOG_HTTP_1 "\r\nReceived unexpected HTTP response code"
#define LOG_HTTP_2 "\r\nHTTP response is not an upgrade to the WebSockets protocol"

// Log messages used for WebSocket
#define LOG_WS_1 "\r\nWebSocket connection established"

// Other log messages
#define LOG_PROGRESSBAR ".."
#define LOG_INLINE "\r\n\t"



// List of logs matching the tests
char* logs[] = {
	LOG_NETWORK_1 LOG_NETWORK_2,

	LOG_NETWORK_1 LOG_NETWORK_3 LOG_SOCKET_1 LOG_SOCKET_2,

	"\r\nLost connection to network" LOG_NETWORK_1 LOG_PROGRESSBAR LOG_NETWORK_2,

	LOG_NETWORK_1 LOG_NETWORK_3 LOG_SOCKET_1 LOG_PROGRESSBAR LOG_SOCKET_2,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_PROGRESSBAR LOG_SOCKET_4,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_SOCKET_5,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_NOT LOG_HTTP_1,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_NOT2 LOG_HTTP_1,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_HALF LOG_SOCKET_6,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_HALF LOG_SOCKET_5,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_404 LOG_HTTP_1,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" LOG_SOCKET_6,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" LOG_SOCKET_5,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION_WRONG "\t\r\n\t" LOG_HTTP_2,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" HTTP_HEADER_UPGRADE_WRONG "\t\r\n\t" LOG_HTTP_2,

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" HTTP_HEADER_UPGRADE "\t" HTTP_HEADER_KEY_WRONG "\t\r\n\t" "\r\nMissing or incorrect WebSocket accept header",

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" HTTP_HEADER_UPGRADE "\t" HTTP_HEADER_KEY "\t" HTTP_HEADER_EXTENSION "\t\r\n\t" "\r\nUnexpected WebSocket Extension header",

	LOG_SOCKET_1 LOG_SOCKET_3 LOG_INLINE HTTP_STATUS "\t" HTTP_HEADER_CONNECTION "\t" HTTP_HEADER_UPGRADE "\t" HTTP_HEADER_KEY "\t" HTTP_HEADER_PROTOCOL "\t\r\n\t" "\r\nUnexpected WebSocket Protocol header"

};

// Runs a test
void testInit() {
	// Prints test id
	printf("" COLOR_BLUE "\nTest: %d\n" COLOR_NORMAL, testState);
	fflush(stdout);

	// Clears log buffer
	log_clear();

	// Run initialize function until error or success
	int8_t state;
	while ((state = verbaleyes_initialize()) == VERBALEYES_INIT_WORKING);

	// Ensures that it delays before continuing after fail
	if (state == -1) {
		time_t start = time(NULL);
		while (start == time(NULL)) {
			if (verbaleyes_initialize() == VERBALEYES_INIT_WORKING) continue;
			fprintf(stderr, "" COLOR_RED "Continued processing when it should not have\n" COLOR_NORMAL);
			numberOfErrors++;
		}
	}

	// Compare log buffer
	if (useShortConf) log_cmp(logs[testState]);

	// Increments test state for next test
	testState++;
	testDroppedConnection = false;
	testReadIndex = 0;
}



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

	// Run tests
	while (testState < (sizeof(logs) / sizeof(logs[0]))) testInit();
}



//!!
int main() {
	//!!
	log_setflags(LOGFLAGBUFFER | LOGFLAGPRINT);

	//!!
	runTests(true);

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
