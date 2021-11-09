#include <stdbool.h> // bool
#include <stdint.h> // int8_t, uint8_t, int16_t, uint16_t, int32_t
#include <string.h> // strcpy, memset, size_t, NULL
#include <time.h> // time, clock, time_t, size_t, NULL
#include <ctype.h> // tolower
#include <stdlib.h> // realloc, free, rand, srand, size_t, NULL
#include <stdio.h> // sprintf, vsnprintf, EOF, size_t, NULL
#include <stdarg.h> // va_list, va_start, va_end

#include <bearssl/bearssl_hash.h> // sha1

#include "./scroll_controller.h"



// Initial buffer size for logging
#define LOGBUFFERLEN 196

// Number of seconds before unfinished configuration input times out
#ifndef CONFIGTIMEOUT
#define CONFIGTIMEOUT 60
#endif

// Number of seconds before unsuccessful connection times out
#ifndef CONNECTINGTIMEOUT
#define CONNECTINGTIMEOUT 10
#endif

// Number of seconds to delay retrying after connection has failed
#ifndef CONNECTIONFAILEDDELAY
#define CONNECTIONFAILEDDELAY 5
#endif

static uint8_t state = 0;
static time_t timeout = 0;



// Prints a string to the serial output with ability to format
static void logprintf(const char* format, ...) {
	// Initializes variadic function
	va_list args;
	va_start(args, format);

	// Formats arguments into buffer
	char buffer[LOGBUFFERLEN];
	const uint8_t len = vsnprintf(buffer, LOGBUFFERLEN, format, args);
	verbaleyes_log(buffer, len);

	// Cleans up variadic function
	va_end(args);
}

// Prints progress bar every second to indicate a process is working and handle timeout errors
static bool showProgressBar() {
	static time_t previous;
	const time_t current = time(NULL);

	// Handles timeout error
	if (current > timeout) return 0;

	// Prints progress bar every second
	if (previous == current) return 1;
	previous = current;
	logprintf(".");
	return 1;
}

// Resets state back with an error message
static int8_t connectionFailToState(const char* msg, const uint8_t backToState) {
	logprintf(msg);
	timeout = time(NULL) + CONNECTIONFAILEDDELAY;
	state = backToState;
	return -1;
}

// Reconnects to socket if unable to get data before timeout
static int8_t socketHadNoData() {
	if (verbaleyes_socket_connected() != 1) return connectionFailToState("\r\nConnection to host closed", 0x90);
	if (time(NULL) < timeout) return 1;
	return connectionFailToState("\r\nResponse from server ended prematurely", 0x90);
}

// Prints progress bar until timeing out if unable to get data
static int8_t socketHadNoDataProgressBar() {
	if (verbaleyes_socket_connected() != 1) return connectionFailToState("\r\nConnection to host closed", 0x90);
	if (showProgressBar()) return 1;
	return connectionFailToState("\r\nDid not get a response from the server", 0x90);
}



// Indication that a match has already succeeded
#define SUCCESSFULMATCH 255

// Matches incoming data character by character
static void matchStr(uint8_t* index, const char c, const char* match) {
	if (*index == SUCCESSFULMATCH) return;
	if (match[*index] == '\0') *index = SUCCESSFULMATCH;
	else if (c == match[*index]) *index += 1;
	else *index = 0;
}



#define WS_PAYLOADLEN_NOTSET 1
#define WS_PAYLOADLEN_EXTENDED 126
#define WS_MASKLEN 4

// Sends a string in a WebSocket frame to the server
static void writeWebSocketFrame(const char* format, ...) {
	// Initializes variadic function
	va_list args;
	va_start(args, format);

	// Creates WebSocket frame for non extended lengt frame with initialized fin bit, rsv bits and opcode
	uint8_t frame[2 + WS_MASKLEN + WS_PAYLOADLEN_EXTENDED] = { 0x81 };

	// Sets payload length for websocket frame
	const uint8_t payloadLen = vsnprintf((char*)frame + 2 + WS_MASKLEN, WS_PAYLOADLEN_EXTENDED, format, args);
	frame[1] = 0x80 | payloadLen;

	// Generates mask
	frame[2] = rand() % 256;
	frame[3] = rand() % 256;
	frame[4] = rand() % 256;
	frame[5] = rand() % 256;

	// Masks payload
	for (uint8_t i = 0; i < payloadLen; i++) {
		frame[i + 2 + WS_MASKLEN] = frame[i + 2 + WS_MASKLEN] ^ frame[(i & 3) + 2];
	}

	// Sends websocket frame
	verbaleyes_socket_write(frame, 2 + WS_MASKLEN + payloadLen);

	// Cleans up variadic function
	va_end(args);
}



// All configurable strings lengths
#define CONF_LEN_SSID           32
#define CONF_LEN_SSIDKEY        63
#define CONF_LEN_HOST           64
#define CONF_LEN_PATH           32
#define CONF_LEN_PROJ           32
#define CONF_LEN_PROJKEY        32

// All configurable items addesses
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



// Reads a config value into a char array
static void confGetStr(const uint16_t addr, const uint16_t len, char* str) {
	for (uint8_t i = 0; i < len; i++) {
		str[i] = verbaleyes_conf_read(addr + i);
		if (str[i] == '\0') return;
	}
	str[len] = '\0';
}

// Reads a config value as a 2 byte int
static uint16_t confGetInt(const uint16_t addr) {
	return (verbaleyes_conf_read(addr) << 8) | (uint8_t)verbaleyes_conf_read(addr + 1);
}



// Structure used to read and write configurable data
// Length of 0 indicates it is a 16bit unsigned integer, -1 indicates 16bit signed integer
struct confItem {
	const char* name; // Name used for configuration
	const int8_t len; // Maxumum length for item
	const uint16_t addr; // Start address in percistent storage
	const uint8_t resetState; // State to go back to when item is updated
	bool nameMatchFailed; // Internally used by configuration parser
};

// Array of all configurable properties
static struct confItem confItems[] = {
	{ "ssid",           CONF_LEN_SSID,        CONF_ADDR_SSID,         0x00 },
	{ "ssidkey",        CONF_LEN_SSIDKEY,     CONF_ADDR_SSIDKEY,      0x00 },
	{ "host",           CONF_LEN_HOST,        CONF_ADDR_HOST,         0x10 },
	{ "port",           0,                    CONF_ADDR_PORT,         0x10 },
	{ "path",           CONF_LEN_PATH,        CONF_ADDR_PATH,         0x10 },
	{ "proj",           CONF_LEN_PROJ,        CONF_ADDR_PROJ,         0x10 },
	{ "projkey",        CONF_LEN_PROJKEY,     CONF_ADDR_PROJKEY,      0x10 },
	{ "speedmin",       -1,                   CONF_ADDR_SPEEDMIN,     0x20 },
	{ "speedmax",       -1,                   CONF_ADDR_SPEEDMAX,     0x20 },
	{ "deadzone",       0,                    CONF_ADDR_DEADZONE,     0x20 },
	{ "callow",         0,                    CONF_ADDR_CALLOW,       0x20 },
	{ "calhigh",        0,                    CONF_ADDR_CALHIGH,      0x20 },
	{ "sensitivity",    0,                    CONF_ADDR_SENS,         0x20 }
};

#define CONFITEMSLEN (sizeof confItems / sizeof confItems[0])

#define FLAGACTIVE 1
#define FLAGCOMMIT 2
#define FLAGFAILED 4
#define FLAGVALUE  8
#define FLAGSIGNED 16

// Updates a configurable property from a stream of characters
bool verbaleyes_configure(const int16_t c) {
	static uint8_t confMatchIndex;
	static uint8_t confFlags = 0;
	static uint8_t confIndex = 0;
	static uint16_t confBuffer = 0;

	switch (c) {
		// Finish matching key on delimiter input and setup to update value
		case '\t':
		case '=': {
			// Falls through to default if not true
			if (confFlags < FLAGVALUE) {
				// Prevents handling failed matches
				if (confFlags & FLAGFAILED) return 1;

				// Finish matching key and reset all items
				for (int8_t i = 0; i < CONFITEMSLEN; i++) {
					if (
						confFlags < FLAGVALUE &&
						!confItems[i].nameMatchFailed &&
						confItems[i].name[confIndex] == '\0'
					) {
						confMatchIndex = i;
						confFlags |= FLAGVALUE;
						logprintf(" ] is now: ");
					}

					confItems[i].nameMatchFailed = 0;
				}

				// Prevents handling value for keys with no match
				if (confFlags < FLAGVALUE) {
					if (confIndex == 0) logprintf("\r\n[");
					logprintf(" ] No matching key");
					confFlags |= FLAGFAILED | FLAGACTIVE;
					return 1;
				}

				// Resets index to be reused for value
				confIndex = 0;
				return 1;
			}
		}
		// Updates configurable value
		default: {
			// Validates incomming key against valid configuration keys
			if (confFlags < FLAGVALUE) {
				// Key handling has failed and remaining characters should be ignored
				if (confFlags & FLAGFAILED) return 1;

				// Special handling for first character in key
				if (confIndex == 0) {
					// Sets timeout for automatically exiting configuration mode
					timeout = time(NULL) + CONFIGTIMEOUT;

					// Ignores everything until next LF if first char indicates comment
					if (c == '#') {
						confFlags |= FLAGFAILED;
						return 1;
					}

					// Initializes new configuration update
					confFlags |= FLAGACTIVE;
					logprintf("\r\n[ ");
				}

				// Invalidates keys that does not match incomming string
				for (int8_t i = 0; i < CONFITEMSLEN; i++) {
					if (
						!confItems[i].nameMatchFailed &&
						c != confItems[i].name[confIndex]
					) {
						confItems[i].nameMatchFailed = 1;
					}
				}
			}
			// Updates string value for matched key
			else if (confIndex < confItems[confMatchIndex].len) {
				verbaleyes_conf_write(confItems[confMatchIndex].addr + confIndex, c);
			}
			// Handles integer input for matched key
			else if (confItems[confMatchIndex].len <= 0 ) {
				// Integer value handling has failed and remaining characters should be ignored
				if (confFlags & FLAGFAILED) return 1;

				// Only accepts a valid numerical representation
				if (c < '0' || c > '9') {
					// Flags input as signed if position of sign and item allows
					if (
						c == '-' &&
						confItems[confMatchIndex].len == -1 &&
						confIndex == 0 &&
						!(confFlags & FLAGSIGNED)
					) {
						confFlags |= FLAGSIGNED;
						logprintf("-");
						return 1;
					}

					// Aborts handling input if it contained invalid characters
					if (confIndex == 0) logprintf("0");
					logprintf("\r\nInvalid input (%c)", c);
					confFlags |= FLAGFAILED;
					return 1;
				}

				// Handles integer overflow, it can only occur with four or more character
				if (confIndex >= 4) {
					// Clamps unsigned integers if it would overflow
					if (confItems[confMatchIndex].len == 0) {
						if (confBuffer > 6553 || (confBuffer == 6553 && c > '5')) {
							confFlags |= FLAGFAILED;
							confBuffer = 65535;
							logprintf("%c\r\nValue was too high and clamped down to maximum value of 65535", c);
							return 1;
						}
					}
					// Clamps signed integers if it would overflow
					else if (confBuffer > 3276 || (confBuffer == 3276 && c > '7')) {
						confFlags |= FLAGFAILED;
						confBuffer = 32767;

						if (confFlags & FLAGSIGNED) {
							logprintf("%c\r\nValue was too low and clamped up to minimum value of -32767", c);
						}
						else {
							logprintf("%c\r\nValue was too high and clamped down to maximum value of 32767", c);
						}

						return 1;
					}
				}

				// Pushes number onto the buffer
				confBuffer = confBuffer * 10 + c - '0';
			}
			// Displays max value length warning message once
			else {
				if (confFlags & FLAGFAILED) return 1;
				confFlags |= FLAGFAILED;
				logprintf("\r\nMaximum input length reached");
				return 1;
			}

			// Character was acceptable and continues reading more data
			confIndex++;
			logprintf("%c", c);
			return 1;
		}
		// Exits on no data read
		case '\0':
		case EOF: {
			// Exit if configuration is not actively being updated
			if (confFlags == 0) return 0;

			// Commits all changed values if commit is required
			if (confFlags == FLAGCOMMIT) {
				if (confFlags & FLAGCOMMIT) verbaleyes_conf_commit();
				logprintf("\r\nConfiguration saved\r\n");
				confFlags = 0;
				return 0;
			}

			// Continues waiting for new data until timeout is reached
			if (time(NULL) < timeout) return 1;
		}
		// Terminates updating configurable data
		case 0x1B:
		case '\n': {
			// Handles termination of value
			if (confFlags >= FLAGVALUE) {
				// Terminates stored string
				if (confItems[confMatchIndex].len > 0) {
					if (confIndex < confItems[confMatchIndex].len) {
						verbaleyes_conf_write(confItems[confMatchIndex].addr + confIndex, '\0');
					}
				}
				// Stores 16 bit integer value
				else {
					// Makes confBuffer negative if flag is set
					if (confFlags & FLAGSIGNED) confBuffer *= -1;

					// Stores 16 bit integer for configuration item
					verbaleyes_conf_write(confItems[confMatchIndex].addr, confBuffer >> 8);
					verbaleyes_conf_write(confItems[confMatchIndex].addr + 1, confBuffer);
					confBuffer = 0;
				}

				// Pulls back state to handle updated value
				if (state > confItems[confMatchIndex].resetState) {
					state = confItems[confMatchIndex].resetState;
				}

				// Resets to handle new keys
				confFlags = FLAGCOMMIT | FLAGACTIVE;
				confIndex = 0;
			}
			// Handles termination of key
			else if (confFlags != 0) {
				// Handles termination for key without a match
				if (confFlags & FLAGFAILED) {
					confIndex = 0;
					confFlags &= ~FLAGFAILED;
				}
				// Handles termination before key was validated
				else if (confIndex != 0) {
					for (int8_t i = 0; i < CONFITEMSLEN; i++) {
						confItems[i].nameMatchFailed = 0;
					}
					logprintf(" ] Aborted");
					confIndex = 0;
				}
				// Clears active flag after double LF
				else {
					confFlags &= ~FLAGACTIVE;
					if (confFlags == 0) logprintf("\r\nConfiguration canceled\r\n");
				}
			}
			// Does not continue processing data
			else {
				return 0;
			}

			// Continues processing more data
			return 1;
		}
		// Ignores these characters
		case 0x7F:
		case '\b':
		case '\f':
		case '\v':
		case '\r': {
			return confFlags != 0;
		}
	}
}



// Global variables for mapping analog scroll input
float speedMapper;
int32_t speedOffset;
int32_t deadzoneSize;
uint16_t jitterSize;

// Global variable for projID to connect to
char projID[CONF_LEN_PROJ + 1];

#define RESINDEXFAILED 65535

// Ensures everything is connected to be able to transmit speed changes to the server
int8_t verbaleyes_initialize() {
	static uint16_t resIndex = 0;
	static uint8_t resMatchIndexes[5];
	static char* buf;

	// Ensure network connection
	switch (state) {
		// Prevents immediately retrying after something fails
		case 0x80:
		case 0x90: {
			if (time(NULL) >= timeout) state &= 0x7F;
			return 1;
		}
		// Reconnects to network if connection is lost
		default: {
			if (verbaleyes_network_connected() == 1) break;
			logprintf("\r\nLost connection to network");
		}
		// Initialize network connection
		case 0x00: {
			// Gets network ssid and key from config
			char ssid[CONF_LEN_SSID + 1];
			confGetStr(CONF_ADDR_SSID, CONF_LEN_SSID, ssid);
			char ssidkey[CONF_LEN_SSIDKEY + 1];
			confGetStr(CONF_ADDR_SSIDKEY, CONF_LEN_SSIDKEY, ssidkey);

			// Prints
			logprintf("\r\nConnecting to SSID: %s...", ssid);

			// Connects to ssid with key
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			verbaleyes_network_connect(ssid, ssidkey);
			state = 0x01;
		}
		// Completes network connection
		case 0x01: {
			// Awaits network connection established
			switch (verbaleyes_network_connected()) {
				// Shows progress bar until network is connected
				case 0: {
					if (showProgressBar()) return 1;
				}
				// Handles timeout error and known fail
				case -1: {
					return connectionFailToState("\r\nFailed to connect to network", 0x80);
				}
			}

			// Prints devices IP address
			logprintf("\r\nNetwork connection established");
			state = 0x10;
		}
	}

	// Ensures connection to socket
	switch (state) {
		// Reconnects to socket if connection is lost
		default: {
			if (verbaleyes_socket_connected() == 1) break;
			logprintf("\r\nLost connection to host");
		}
		// Initialize socket connection
		case 0x10: {
			// Gets host and port from config
			buf = (char*)realloc(buf, CONF_LEN_HOST + 1);
			if (buf == NULL) logprintf("\r\nERROR: realloc failed\r\n");
			confGetStr(CONF_ADDR_HOST, CONF_LEN_HOST, buf);
			const uint16_t port = confGetInt(CONF_ADDR_PORT);

			// Prints
			logprintf("\r\nConnecting to host: %s:%u...", buf, port);

			// Connects to socket at host
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			verbaleyes_socket_connect(buf, port);
			state = 0x11;
		}
		// Completes socket connection
		case 0x11: {
			// Awaits socket connectin established
			switch (verbaleyes_socket_connected()) {
				// Shows progress bar until socket is connected
				case 0: {
					if (showProgressBar()) return 1;
				}
				// Handles timeout error and know fail
				case -1: {
					return connectionFailToState("\r\nFailed to connect to host", 0x90);
				}
			}
		}
		// Sends http request to use websocket protocol
		case 0x12: {
			// Gets path to use on host
			char path[CONF_LEN_PATH + 1];
			confGetStr(CONF_ADDR_PATH, CONF_LEN_PATH, path);

			// Prints
			logprintf("\r\nAccessing WebSocket server at %s...", path);

			// Sets random seed
			srand(clock());

			// Generates websocket key
			const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			char key[25];
			for (uint8_t i = 0; i < 21; i++) {
				key[i] = table[rand() % 64];
			}
			key[21] = table[rand() % 4 * 16];
			key[22] = '=';
			key[23] = '=';
			key[24] = '\0';

			// Flushes any data existing in sockets read buffer
			while (verbaleyes_socket_read() != EOF);

			// Sends HTTP request to setup WebSocket connection with host
			char req[4 + CONF_LEN_PATH + 17 + CONF_LEN_HOST + 89 + 24 + 4 + 1];
			uint8_t reqlen = sprintf(
				req,
				"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: %s\r\n\r\n",
				path,
				buf,
				key
			);
			verbaleyes_socket_write((uint8_t*)req, reqlen);

			// Creates websocket accept header to compare against
			buf = (char*)realloc(buf, 22 + 28 + 2 + 1);
			if (buf == NULL) logprintf("\r\nERROR: realloc failed\r\n");
			strcpy(buf, "sec-websocket-accept: ");
			br_sha1_context ctx;
			br_sha1_init(&ctx);
			br_sha1_update(&ctx, key, 24);
			br_sha1_update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
			uint8_t hash[21];
			br_sha1_out(&ctx, hash);
			hash[20] = 0;
			for (uint8_t i = 0; i < 21; i += 3) {
				const uint8_t offset = i / 3;
				buf[i + 22 + offset] = table[hash[i] >> 2];
				buf[i + 1 + 22 + offset] = table[((hash[i] & 0x03) << 4) | hash[i + 1] >> 4];
				buf[i + 2 + 22 + offset] = table[(hash[i + 1] & 0x0f) << 2 | hash[i + 2] >> 6];
				buf[i + 3 + 22 + offset] = table[hash[i + 2] & 0x3f];
			}
			strcpy(buf + 22 + 27, "=\r\n");

			// Sets timeout value for awaiting http response
			timeout = time(NULL) + CONNECTINGTIMEOUT;

			// Sets up to read and verify http response
			state = 0x13;
		}
		// Validates first HTTP status-line character
		case 0x13: {
			const int16_t c = verbaleyes_socket_read();

			// Shows progress bar until socket starts receiving data
			if (c == EOF) return socketHadNoDataProgressBar();

			// Validates first character for status-line and moves on to validate the rest
			logprintf("\r\n\t%c", c);
			resIndex = (toupper(c) == 'H') ? 1 : RESINDEXFAILED;
			state = 0x14;
		}
		// Validates HTTP status-line
		case 0x14: {
			while (resIndex != 12) {
				const int16_t c = verbaleyes_socket_read();

				// Handles incorrect status code, timeout and socket close error
				if (c == EOF) {
					if (resIndex != RESINDEXFAILED) return socketHadNoData();
					return connectionFailToState("\r\nReceived unexpected HTTP response code", 0x90);
				}

				// Prints HTTP status-line
				if (c == '\n') {
					logprintf("\r\n\t");
				}
				else {
					logprintf("%c", c);
				}

				// Prints entire HTTP response before handling unexpected HTTP response code
				if (resIndex == RESINDEXFAILED) continue;

				// Validates incoming data for http status-line
				if (toupper(c) == "HTTP/1.1 101"[resIndex]) {
					resIndex++;
				}
				// Failed to validate http status-line, abort after printing entire request
				else {
					resIndex = RESINDEXFAILED;
				}
			}

			// Successfully validated status-line and sets up to validate http headers
			memset(resMatchIndexes, 0, sizeof resMatchIndexes);
			state = 0x15;
		}
		// Validates HTTP headers
		case 0x15: {
			// Validate headers until EOF
			int16_t c;
			while ((c = verbaleyes_socket_read()) != EOF) {
				// Analyzes HTTP headers up to end of head
				matchStr((uint8_t*)&resIndex, c, "\r\n\r\n");

				// Prints HTTP headers
				if (c == '\n') {
					logprintf("\n\t");
				}
				else {
					logprintf("%c", c);
				}

				// Matches the incoming HTTP response against required and illigal substrings
				const char lowerc = tolower(c);
				matchStr(&resMatchIndexes[0], lowerc, "connection: upgrade\r\n");
				matchStr(&resMatchIndexes[1], lowerc, "upgrade: websocket\r\n");
				matchStr(&resMatchIndexes[2], (resMatchIndexes[2] <= 20) ? lowerc : c, buf);
				matchStr(&resMatchIndexes[3], lowerc, "sec-websocket-extensions: ");
				matchStr(&resMatchIndexes[4], lowerc, "sec-webSocket-protocol: ");
			}

			// Handles timeout and socket close error if end of headers was not reached
			if (resIndex != 4) return socketHadNoData();

			// Requires "Connection" header with "Upgrade" value and "Upgrade" header with "websocket" value
			if (!resMatchIndexes[0] || !resMatchIndexes[1]) {
				return connectionFailToState("\r\nHTTP response is not an upgrade to the WebSockets protocol", 0x90);
			}
			// Requires WebSocket accept header with correct value
			else if (!resMatchIndexes[2]) {
				return connectionFailToState("\r\nMissing or incorrect WebSocket accept header", 0x90);
			}
			// Checks for non-requested WebSocket extension header
			else if (resMatchIndexes[3]) {
				return connectionFailToState("\r\nUnexpected WebSocket Extension header", 0x90);
			}
			// Checks for non-requested WebSocket protocol header
			else if (resMatchIndexes[4]) {
				return connectionFailToState("\r\nUnexpected WebSocket Protocol header", 0x90);
			}

			// Frees up allocated buffer
			free(buf);
			buf = NULL;

			// Successfully validated http headers
			logprintf("\r\nWebSocket connection established");
		}
		// Connect to verbalEyes project
		case 0x16: {
			// Gets project and project key from config
			confGetStr(CONF_ADDR_PROJ, CONF_LEN_PROJ, projID);
			char projkey[CONF_LEN_PROJKEY + 1];
			confGetStr(CONF_ADDR_PROJKEY, CONF_LEN_PROJKEY, projkey);

			// Prints
			logprintf("\r\nConnecting to project: %s...", projID);

			// Sends VerbalEyes project authentication request
			writeWebSocketFrame("[{\"id\": \"%s\", \"auth\": \"%s\"}]", projID, projkey);

			// Sets timeout value for awaiting websocket response
			timeout = time(NULL) + CONNECTINGTIMEOUT;

			// Sets up to read and verify websocket response
			resIndex = 0;
			state = 0x17;
		}
		// Validates WebSocket opcode for authentication
		case 0x17: {
			const int16_t c = verbaleyes_socket_read();

			// Shows progress bar until socket starts receiving data
			if (c == EOF) return socketHadNoDataProgressBar();

			// Makes sure this is an unfragmented WebSocket frame in text format
			if (c != 0x81) {
				return connectionFailToState("\r\nReceived response data is either not a WebSocket frame or uses an unsupported WebSocket feature", 0x90);
			}

			// Sets up to read WebSocket payload length
			resIndex = WS_PAYLOADLEN_NOTSET;
			state = 0x18;
		}
		// Gets length of WebSocket payload for authentication
		case 0x18: {
			while (1) {
				const int16_t c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) return socketHadNoData();

				// Gets payload length and continues if extended payload length is used
				if (resIndex == WS_PAYLOADLEN_NOTSET) {
					// Server is not allowed to mask messages sent to the client according to the spec
					if (c & 0x80) return connectionFailToState("\r\nReveiced a masked frame which is not allowed", 0x90);

					// Gets payload length without mask bit
					resIndex = c & 0x7F;

					// Moves on if payload length was defined in one byte
					if (resIndex < WS_PAYLOADLEN_EXTENDED) break;

					// Aborts if payload length requires more than the 16 bits available in resIndex
					if (resIndex == 127) return connectionFailToState("\r\nWebsocket frame was unexpectedly long", 0x90);
				}
				// Gets first byte of extended payload length
				else if (resIndex == WS_PAYLOADLEN_EXTENDED) {
					resIndex = c << 8;
				}
				// Adds second byte of extended payload length and moves on
				else {
					resIndex |= c;
					break;
				}
			}

			// Sets up to read WebSocket payload
			logprintf("\r\nReceived authentication response:\r\n\t");
			resMatchIndexes[0] = 0;
			state = 0x19;
		}
		// Validates WebSocket payload for authentication
		case 0x19: {
			// Reads entire WebSocket authentication response
			while (resIndex) {
				const int16_t c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) return socketHadNoData();

				// Prints entire WebSocket payload
				if (c == '\n') {
					logprintf("\n\t");
				}
				else {
					logprintf("%c", c);
				}

				// Makes sure authentication was successful
				if (resMatchIndexes[0] != SUCCESSFULMATCH && c > ' ') matchStr(&resMatchIndexes[0], c, "\"auth\":true");
				resIndex--;
			}

			// Validates authentication
			if (resMatchIndexes[0] != SUCCESSFULMATCH) return connectionFailToState("\r\nAuthentication failed", 0x90);

			// Moves on for successful authentication
			logprintf("\r\nAuthenticated");
		}
		// Sets global values used for updating speed
		case 0x20: {
			// Gets deadzone percentage value from config
			const uint8_t deadzone = verbaleyes_conf_read(CONF_ADDR_DEADZONE + 1);

			// Gets minimum and maximum speed from config
			const int16_t speedMin = confGetInt(CONF_ADDR_SPEEDMIN);
			const int16_t speedMax = confGetInt(CONF_ADDR_SPEEDMAX);

			// Gets calibration start and end point to use on analog read value
			const uint16_t speedCalLow = confGetInt(CONF_ADDR_CALLOW);
			const uint16_t speedCalHigh = confGetInt(CONF_ADDR_CALHIGH);

			// Gets sensitivity value based on calibration range
			const uint16_t sensitivity = confGetInt(CONF_ADDR_SENS);

			// Sets helper values to use when mapping analog read value to new range
			uint8_t deadzoneCapped = (deadzone > 99) ? 99 : deadzone;
			deadzoneSize = (speedMax - speedMin) * 100 * deadzone / (100 - deadzoneCapped);
			float speedSize = (speedMax - speedMin) * 100 + deadzoneSize;
			speedMapper = speedSize / (speedCalHigh - speedCalLow);
			speedOffset = speedMin * 100 - (speedCalLow * speedMapper);
			jitterSize = sensitivity * speedMapper;

			// Prints settings
			logprintf(
				"\r\nSetting up speed reader with:\r\n\tMaximum speed at: %i\r\n\tMinimum speed at: %i\r\n\tDeadzone at: %d%%\r\n\tCalibration low at: %u\r\n\tCalibration high at: %u\r\n\tSensitivity at: %d\r\n",
				speedMax,
				speedMin,
				deadzone,
				speedCalLow,
				speedCalHigh,
				sensitivity
			);

			// Sets state to be outside range now that it is done
			state = 0xFF;
		}
	}

	// Allows caller function to continue past this function
	return 0;
}

// Sends remapped analog speed reading to the server
void verbaleyes_setspeed(const uint16_t value) {
	static int32_t speed;

	// Maps analog input value to conf range
	int32_t mappedValue = value * speedMapper + speedOffset;

	// Shifts mapped value above deadzone
	if (mappedValue > deadzoneSize) {
		mappedValue -= deadzoneSize;
	}
	// Clamps value to deadzone around 0 mark
	else if (mappedValue >= 0) {
		if (speed == 0) return;
		mappedValue = 0;
	}

	// Supresses updating speed if it has not changed enough unless it is updated to zero
	if (mappedValue != 0 && mappedValue <= speed + jitterSize && mappedValue >= speed - jitterSize) return;
	speed = mappedValue;

	// Sends new speed to the server
	writeWebSocketFrame("[{\"id\": \"%s\", \"scrollSpeed\": %.2f}]", projID, (float)speed / 100);

	// Prints new speed
	logprintf("\r\nSpeed has been updated to: %.2f", (float)speed / 100);
}

// Tells server to reset position to 0 when button is pressed
void verbaleyes_resetoffset(const bool value) {
	static bool buttonState = 1;

	// Only sends data on button down event
	if (value == buttonState) return;
	buttonState = value;
	if (value == 0) return;

	// Sends message to server
	writeWebSocketFrame("[{\"id\": \"%s\", \"scrollOffset\": 0}]", projID);

	// Prints
	logprintf("\r\nScroll position has been set to: 0");
}
