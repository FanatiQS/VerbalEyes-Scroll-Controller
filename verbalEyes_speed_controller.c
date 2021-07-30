#include <stdbool.h> // bool
#include <stdint.h> // int8_t uint8_t int16_t uint16_t int32_t uint32_t
#include <string.h> // strlen, strcpy, memset, memcpy, size_t, NULL
#include <time.h> // time, clock, time_t, size_t, NULL
#include <ctype.h> // tolower
#include <stdlib.h> // realloc, free, rand, srand, size_t, NULL
#include <stdio.h> // sprintf, vsprintf, vsnprintf, EOF, size_t, NULL
#include <stdarg.h> // va_list, va_start, va_end

#include <bearssl/bearssl_hash.h> // sha1

#include "./verbalEyes_speed_controller.h"



// Initial buffer size for logging
#define LOGBUFFERLEN 45

// Number of seconds before unfinished configuration input times out
#ifndef CONFIGTIMEOUT
#define CONFIGTIMEOUT 5
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
	const size_t len = vsnprintf(buffer, LOGBUFFERLEN, format, args);
	if (len < LOGBUFFERLEN) {
		verbaleyes_log(buffer, len);
	}
	// Retries with known length if buffer was too small
	else {
		va_start(args, format);
		char buffer2[len + 1];
		vsnprintf(buffer2, len + 1, format, args);
		verbaleyes_log(buffer2, len);
	}

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
	return CONNECTIONFAILED;
}

// Reconnects to socket if unable to get data before timeout
static int8_t socketHadNoData() {
	if (!verbaleyes_socket_connected()) return connectionFailToState("\r\nConnection to host closed", 0x82);
	if (time(NULL) < timeout) return CONNECTING;
	return connectionFailToState("\r\nResponse from server ended prematurely", 0x82);
}

// Prints progress bar until timeing out if unable to get data
static int8_t socketHadNoDataProgressBar() {
	if (!verbaleyes_socket_connected()) return connectionFailToState("\r\nConnection to host closed", 0x82);
	if (showProgressBar()) return CONNECTING;
	return connectionFailToState("\r\nDid not get a response from the server", 0x82);
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

// Inserts randomly generated mask and uses it to mask payload for WebSocket frame
static void maskWebSocketFrame(uint8_t* frame, const size_t len) {
	// Generates mask
	frame[0] = rand() % 256;
	frame[1] = rand() % 256;
	frame[2] = rand() % 256;
	frame[3] = rand() % 256;

	// Masks payload
	for (size_t i = WS_MASKLEN; i < len; i++) {
		frame[i] = frame[i] ^ frame[i - WS_MASKLEN & 3];
	}
}

// Sends a string in a WebSocket frame to the server
static void writeWebSocketFrame(const char* format, ...) {
	// Initializes variadic function
	va_list args;
	va_start(args, format);

	// Creates WebSocket frame for non extended frame
	uint8_t frame[2 + WS_MASKLEN + WS_PAYLOADLEN_EXTENDED];
	const size_t payloadLen = vsnprintf((char*)frame + 2 + WS_MASKLEN, WS_PAYLOADLEN_EXTENDED, format, args);

	// Handles non extended frame
	if (payloadLen < WS_PAYLOADLEN_EXTENDED) {
		// Sets fin bit, reserved buts, opcode and payload length for websocket frame
		frame[0] = 0x81;
		frame[1] = 0x80 | payloadLen;

		// Generates mask, masks payload and sends frame
		maskWebSocketFrame(frame + 2, payloadLen + WS_MASKLEN);
		verbaleyes_socket_write(frame, 2 + WS_MASKLEN + payloadLen);
	}
	// Handles extended WebSocket frames
	else {
		// Creates separate buffer for extended frame
		uint8_t frame2[4 + WS_MASKLEN + payloadLen + 1];
		vsnprintf((char*)frame2 + 4 + WS_MASKLEN, payloadLen + 1, format, args);

		// Sets fin bit, reserved bits, opcode and payload length for websocket frame
		frame2[0] = 0x81;
		frame2[1] = 0x80 | WS_PAYLOADLEN_EXTENDED;
		frame2[2] = payloadLen >> 8;
		frame2[3] = payloadLen;

		// Generates mask, masks payload and sends frame
		maskWebSocketFrame(frame2 + 4, payloadLen + WS_MASKLEN);
		verbaleyes_socket_write(frame2, 4 + WS_MASKLEN + payloadLen);
	}

	// Cleans up variadic function
	va_end(args);
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

// All configurable properties
static struct confItem conf_ssid =       	{	"ssid",       	32, 	0,  	0 	};
static struct confItem conf_ssidkey =    	{	"ssidkey",    	63, 	32, 	0 	};
static struct confItem conf_host =       	{	"host",       	64, 	95, 	2 	};
static struct confItem conf_port =       	{	"port",       	0,  	159,	2 	};
static struct confItem conf_path =       	{	"path",       	32, 	161,	2 	};
static struct confItem conf_proj =       	{	"proj",       	32, 	193,	2 	};
static struct confItem conf_projkey =    	{	"projkey",    	32, 	225,	2 	};
static struct confItem conf_speedmin =   	{	"speedmin",   	-1, 	257,	12	};
static struct confItem conf_speedmax =   	{	"speedmax",   	-1, 	259,	12	};
static struct confItem conf_deadzone =   	{	"deadzone",   	0,  	261,	12	};
static struct confItem conf_callow =     	{	"callow",     	0,  	263,	12	};
static struct confItem conf_calhigh =    	{	"calhigh",    	0,  	265,	12	};
static struct confItem conf_sensitivity =	{	"sensitivity",	0,  	267,	12	};



// Reads a config value into a char array
static void confGetStr(const struct confItem item, char* str) {
	for (uint8_t i = 0; i < item.len; i++) {
		str[i] = verbaleyes_conf_read(item.addr + i);
		if (str[i] == '\0') return;
	}
	str[item.len] = '\0';
}

// Reads a config value as a 2 byte int
static uint16_t confGetInt(const struct confItem item) {
	return (verbaleyes_conf_read(item.addr) << 8) | verbaleyes_conf_read(item.addr + 1);
}



// Array of all configurable properties
static struct confItem *confItems[] = {
	&conf_ssid,
	&conf_ssidkey,
	&conf_host,
	&conf_port,
	&conf_path,
	&conf_proj,
	&conf_projkey,
	&conf_speedmin,
	&conf_speedmax,
	&conf_deadzone,
	&conf_callow,
	&conf_calhigh,
	&conf_sensitivity
};

#define CONFITEMSLEN sizeof confItems / sizeof confItems[0]

#define FLAGACTIVE 1
#define FLAGCOMMIT 2
#define FLAGFAILED 4
#define FLAGVALUE 8

// Updates a configurable property from a stream of characters
bool updateConfig(const int16_t c) {
	static struct confItem* item;
	static uint8_t confFlags = 0;
	static uint8_t confIndex = 0;
	static uint16_t confBuffer = 0;
	static bool confBufferSigned = 0;

	switch (c) {
		// Finish matching key on delimiter input and setup to update value
		case '\t':
		case '=': {
			// Falls through to default if not true
			if (confFlags < FLAGVALUE) {
				// Prevents validating key if delimiter is first character
				if (confFlags == 0) return 0;

				// Prevents handling failed matches
				if (confFlags & FLAGFAILED) return 1;

				// Finish matching key and reset all items
				for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
					if (
						confFlags < FLAGVALUE &&
						!confItems[i]->nameMatchFailed &&
						confItems[i]->name[confIndex] == '\0'
					) {
						item = confItems[i];
						confFlags |= FLAGVALUE;
						logprintf(" ] is now: ");
					}

					confItems[i]->nameMatchFailed = 0;
				}

				// Prevents handling value for keys with no match
				if (confFlags < FLAGVALUE) {
					if (confIndex == 0) logprintf("\r\n[");
					logprintf(" ] No matching key");
					confFlags |= FLAGFAILED;
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

				// Initialize new configuration update
				if (confIndex == 0) {
					// Ignores everything until next LF if first char indicates comment
					if (c == '#') {
						confFlags |= FLAGFAILED;
						return 1;
					}

					confFlags |= FLAGACTIVE;
					logprintf("\r\n[ ");
				}

				// Invalidates keys that does not match incomming string
				for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
					if (
						!confItems[i]->nameMatchFailed &&
						c != confItems[i]->name[confIndex]
					) {
						confItems[i]->nameMatchFailed = 1;
					}
				}
			}
			// Updates string value for matched key
			else if (confIndex < item->len) {
				verbaleyes_conf_write(item->addr + confIndex, c);
			}
			// Handles integer input for matched key
			else if (item->len <= 0 ) {
				// Integer value handling has failed and remaining characters should be ignored
				if (confFlags & FLAGFAILED) return 1;

				// Only accepts a valid numerical representation
				if (c < '0' || c > '9') {
					// Flags input as signed if position of sign and item allows
					if (c == '-' && item->len == -1 && confIndex == 0 && !confBufferSigned) {
						confBufferSigned = 1;
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
					if (item->len == 0) {
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

						if (confBufferSigned) {
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
			if (confFlags != FLAGCOMMIT) return 1;

			// Continues waiting for new data until timeout is reached
			if (time(NULL) < timeout) return 1;
		}
		// Terminates updating configurable data
		case 0x1B:
		case '\n': {
			// Handles termination of value
			if (confFlags >= FLAGVALUE) {
				// Terminates stored string
				if (item->len > 0) {
					if (confIndex < item->len) verbaleyes_conf_write(item->addr + confIndex, '\0');
				}
				// Stores 16 bit integer value
				else {
					// Makes confBuffer negative if flag is set
					if (confBufferSigned) {
						confBufferSigned = 0;
						confBuffer *= -1;
					}

					// Stores 16 bit integer for configuration item
					verbaleyes_conf_write(item->addr, confBuffer >> 8);
					verbaleyes_conf_write(item->addr + 1, confBuffer);
					confBuffer = 0;
				}

				// Pulls back state to handle updated value
				if (state > item->resetState) state = item->resetState;

				// Resets to handle new keys
				confFlags = FLAGCOMMIT;
				timeout = time(NULL) + CONFIGTIMEOUT;
				confIndex = 0;
				return 1;
			}
			// Handles termination of key
			else if (confFlags != 0) {
				// Handles termination for key without a match
				if (confFlags & FLAGFAILED) {
					confIndex = 0;
					confFlags ^= (FLAGFAILED | FLAGACTIVE);
				}
				// Handles termination before key was validated
				else if (confIndex != 0) {
					for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
						confItems[i]->nameMatchFailed = 0;
					}
					logprintf(" ] Aborted");
					confIndex = 0;
					confFlags ^= FLAGACTIVE;
				}
				// Commits all changed values and exits configuration handling
				else {
					verbaleyes_conf_commit();
					logprintf("\r\nDone\r\n");
					confFlags = 0;
				}

				return 1;
			}

			return 0;
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
int32_t speedSize;
int16_t speedMapper;
int32_t speedComp;
int32_t deadzoneSize;
int32_t jitterSize;

#define RESINDEXFAILED 65535

// Ensures everything is connected to be able to transmit speed changes to the server
int8_t ensureConnection() {
	static uint16_t resIndex = 0;
	static uint8_t resMatchIndexes[5];
	static char* buf;

	// Ensure network connection
	switch (state) {
		// Prevents immediately retrying after something fails
		case 0x80:
		case 0x82: {
			if (time(NULL) >= timeout) state &= 0x7F;
			return CONNECTING;
		}
		// Reconnects to network if connection is lost
		default: {
			if (verbaleyes_network_connected()) break;
			logprintf("\r\nLost connection to network");
		}
		// Initialize network connection
		case 0: {
			// Gets network ssid and key from config
			char ssid[conf_ssid.len + 1];
			confGetStr(conf_ssid, ssid);
			char ssidkey[conf_ssidkey.len + 1];
			confGetStr(conf_ssidkey, ssidkey);

			// Prints
			logprintf("\r\nConnecting to SSID: %s...", ssid);

			// Connects to ssid with key
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			verbaleyes_network_connect(ssid, ssidkey);
			state = 1;
		}
		// Completes network connection
		case 1: {
			// Awaits network connection established
			if (!verbaleyes_network_connected()) {
				// Shows progress bar until network is connected
				if (showProgressBar()) return CONNECTING;

				// Handles timeout error
				return connectionFailToState("\r\nFailed to connect to network", 0x80);
			}

			// Prints devices IP address
			const uint32_t ip = verbaleyes_network_getip();
			logprintf("\r\nIP address: %u.%u.%u.%u", (uint8_t)(ip), (uint8_t)(ip >> 8), (uint8_t)(ip >> 16), (uint8_t)(ip >> 24));
			state = 2;
		}
	}

	// Ensures connection to socket
	switch (state) {
		// Reconnects to socket if connection is lost
		default: {
			if (verbaleyes_socket_connected()) break;
			logprintf("\r\nLost connection to host");
		}
		// Initialize socket connection
		case 2: {
			// Gets host and port from config
			buf = (char*)realloc(buf, conf_host.len + 1);
			if (buf == NULL) logprintf("\r\nERROR: realloc failed\r\n");
			confGetStr(conf_host, buf);
			const uint16_t port = confGetInt(conf_port);

			// Prints
			logprintf("\r\nConnecting to host: %s:%u...", buf, port);

			// Connects to socket at host
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			verbaleyes_socket_connect(buf, port);
			state = 3;
		}
		// Completes socket connection
		case 3: {
			// Awaits socket connectin established
			if (!verbaleyes_socket_connected()) {
				// Shows progress bar until socket is connected
				if (showProgressBar()) return CONNECTING;

				// Handles timeout error
				return connectionFailToState("\r\nFailed to connect to host", 0x82);
			}
		}
		// Sends http request to use websocket protocol
		case 4: {
			// Gets path to use on host
			char path[conf_path.len + 1];
			confGetStr(conf_path, path);

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
			char req[4 + strlen(path) + 98 + 24 + 4 + 1];
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
			state = 5;
		}
		// Validates first HTTP status-line character
		case 5: {
			const int16_t c = verbaleyes_socket_read();

			// Shows progress bar until socket starts receiving data
			if (c == EOF) return socketHadNoDataProgressBar();

			// Validates first character for status-line and moves on to validate the rest
			logprintf("\r\n\t%c", c);
			resIndex = (tolower(c) == 'h') ? 1 : RESINDEXFAILED;
			state = 6;
		}
		// Validates HTTP status-line
		case 6: {
			while (resIndex != 12) {
				const int16_t c = verbaleyes_socket_read();

				// Handles incorrect status code, timeout and socket close error
				if (c == EOF) {
					if (resIndex == RESINDEXFAILED) {
						return connectionFailToState("\r\nReceived unexpected HTTP response code", 0x82);
					}
					return socketHadNoData();
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
				if (tolower(c) == "http/1.1 101"[resIndex]) {
					resIndex++;
				}
				// Failed to validate http status-line, abort after printing entire request
				else {
					resIndex = RESINDEXFAILED;
				}
			}

			// Successfully validated status-line and sets up to validate http headers
			memset(resMatchIndexes, 0, sizeof resMatchIndexes);
			state = 7;
		}
		// Validates HTTP headers
		case 7: {
			// Validate headers until EOF
			int16_t c;
			while ((c = verbaleyes_socket_read()) != EOF) {
				// Analyzes HTTP headers up to end of head
				matchStr((uint8_t*)&resIndex, c, "\r\n\r\n");

				// Prints HTTP headers
				if (c == '\n') {
					logprintf("\r\n\t");
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

			// Frees up allocated buffer
			free(buf);
			buf = NULL;

			// Requires "Connection" header with "Upgrade" value and "Upgrade" header with "websocket" value
			if (!resMatchIndexes[0] || !resMatchIndexes[1]) {
				return connectionFailToState("\r\nHTTP response is not an upgrade to the WebSockets protocol", 0x82);
			}
			// Requires WebSocket accept header with correct value
			else if (!resMatchIndexes[2]) {
				return connectionFailToState("\r\nMissing or incorrect WebSocket accept header", 0x82);
			}
			// Checks for non-requested WebSocket extension header
			else if (resMatchIndexes[3]) {
				return connectionFailToState("\r\nUnexpected WebSocket Extension header", 0x82);
			}
			// Checks for non-requested WebSocket protocol header
			else if (resMatchIndexes[4]) {
				return connectionFailToState("\r\nUnexpected WebSocket Protocol header", 0x82);
			}

			// Successfully validated http headers
			logprintf("\r\nWebSocket connection established");
		}
		// Connect to verbalEyes project
		case 8: {
			// Gets project and project key from config
			char proj[conf_proj.len + 1];
			confGetStr(conf_proj, proj);
			char projkey[conf_projkey.len + 1];
			confGetStr(conf_projkey, projkey);

			// Prints
			logprintf("\r\nConnecting to project: %s...", proj);

			// Sends VerbalEyes project authentication request
			writeWebSocketFrame("{\"_core\": {\"auth\": {\"id\": \"%s\", \"key\": \"%s\"}}}", proj, projkey);

			// Sets timeout value for awaiting websocket response
			timeout = time(NULL) + CONNECTINGTIMEOUT;

			// Sets up to read and verify websocket response
			resIndex = 0;
			state = 9;
		}
		// Validates WebSocket opcode for authentication
		case 9: {
			const int16_t c = verbaleyes_socket_read();

			// Shows progress bar until socket starts receiving data
			if (c == EOF) return socketHadNoDataProgressBar();

			// Makes sure this is an unfragmented WebSocket frame in text format
			if (c != 0x81) {
				return connectionFailToState("\r\nReceived response data is either not a WebSocket frame or uses an unsupported WebSocket feature", 0x82);
			}

			// Sets up to read WebSocket payload length
			resIndex = WS_PAYLOADLEN_NOTSET;
			state = 10;
		}
		// Gets length of WebSocket payload for authentication
		case 10: {
			while (1) {
				const int16_t c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) return socketHadNoData();

				// Gets payload length and continues if extended payload length is used
				if (resIndex == WS_PAYLOADLEN_NOTSET) {
					// Server is not allowed to mask messages sent to the client according to the spec
					if (c & 0x80) return connectionFailToState("\r\nReveiced a masked frame which is not allowed", 0x82);

					// Gets payload length without mask bit
					resIndex = c & 0x7F;

					// Moves on if payload length was defined in one byte
					if (resIndex < WS_PAYLOADLEN_EXTENDED) break;

					// Aborts if payload length requires more than the 16 bits available in resIndex
					if (resIndex == 127) return connectionFailToState("\r\nWebsocket frame was unexpectedly long", 0x82);
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
			state = 11;
		}
		// Validates WebSocket payload for authentication
		case 11: {
			// Reads entire WebSocket authentication response
			while (resIndex) {
				const int16_t c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) return socketHadNoData();

				// Prints entire WebSocket payload
				if (c == '\n') {
					logprintf("\r\n\t");
				}
				else {
					logprintf("%c", c);
				}

				// Makes sure authentication was successful
				if (resMatchIndexes[0] != SUCCESSFULMATCH) matchStr(&resMatchIndexes[0], c, "authed");
				resIndex--;
			}

			// Validates authentication
			if (!resMatchIndexes[0]) return connectionFailToState("\r\nAuthentication failed", 0x82);

			// Moves on for successful authentication
			logprintf("\r\nAuthenticated");
		}
		// Sets global values used for updating speed
		case 12: {
			// Gets deadzone an sensitivity percentage values from config
			const uint8_t deadzone = confGetInt(conf_deadzone);
			const uint8_t sensitivity = confGetInt(conf_sensitivity);

			// Gets minimum and maximum speed from config
			const int16_t speedMin = confGetInt(conf_speedmin);
			const int16_t speedMax = confGetInt(conf_speedmax);

			// Gets calibration start and end point to use on analog read value
			const uint16_t speedCalLow = confGetInt(conf_callow);
			const uint16_t speedCalHigh = confGetInt(conf_calhigh);

			// Sets helper values to use when mapping analog read value to new range
			speedSize = (speedMax - speedMin) * (100 + deadzone * 2);
			speedMapper = speedCalHigh - speedCalLow;
			speedComp = speedMin * 100;
			deadzoneSize = speedSize * deadzone / 100;
			jitterSize = speedSize * sensitivity / 100;

			// Prints settings
			logprintf(
				"\r\nSetting up speed reader with:\r\n\tMinimum speed at: %i\r\n\tMaximum speed at: %i\r\n\tDeadzone at: %.0f%%\r\n\tSensitivity at: %.0f%%\r\n\tCalibration low at: %u\r\n\tCalibration high: %u\r\n",
				speedMin,
				speedMax,
				deadzone,
				sensitivity,
				speedCalLow,
				speedCalHigh
			);

			// Sets state to be outside range now that it is done
			state = 0xFF;
		}
	}

	// Allows caller function to continue past this function
	return CONNECTED;
}

// Sends remapped analog speed reading to the server
void updateSpeed(const uint16_t value) {
	static int32_t speed;

	// Maps analog input value to conf range
	int32_t mappedValue = value * speedSize / speedMapper + speedComp;

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
	writeWebSocketFrame("{\"_core\": {\"doc\": {\"id\": \"test\", \"speed\": %.2f}}}", (float)speed / 100);

	// Prints new speed
	logprintf("\r\nSpeed has been updated to: %.2f", (float)speed / 100);
}

// Tells server to reset position to 0 when button is pressed
void jumpToTop(const bool value) {
	static bool buttonState = 1;

	// Only sends data on button down event
	if (value == buttonState) return;
	buttonState = value;
	if (value == 0) return;

	// Sends message to server
	writeWebSocketFrame("{\"_core\": {\"doc\": {\"id\": \"test\", \"offset\": 0}}}");

	// Prints
	logprintf("\r\nScroll position has been set to: 0");
}
