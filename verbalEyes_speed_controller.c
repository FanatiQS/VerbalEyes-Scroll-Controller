#include <stdbool.h> // bool
#include <stdint.h> // uint8_t uint16_t uint32_t int8_t int16_t
#include <string.h> // strlen, strcpy, memset
#include <time.h> // time, clock
#include <ctype.h> // tolower
#include <stdlib.h> // rand, srand
#include <stdio.h> // sprintf, svnprintf, vsprintf, EOF
#include <stdarg.h> // va_list, va_start, va_end

#include <bearssl/bearssl_hash.h> // sha1
#include <libb64/cencode.h> // base64

#include "./verbalEyes_speed_controller.h"



#define TAB "    "

static uint16_t state = 0;
static uint32_t timeout = 0;



// Prints a string to the serial output with ability to format
static void logprintf(const char* format, ...) {
	va_list args;
    va_start(args, format);
	const uint32_t len = vsnprintf(NULL, 0, format, args);
	char buffer[len + 1];
	va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
	verbaleyes_log(buffer, len);
}

// Prints progress bar every second to indicate a process is working and handle timeout errors
static bool showProgressBar() {
	static uint32_t previous;
	const uint32_t current = time(NULL);

	// Handle timeout error
	if (current > timeout) return 0;

	// Prints progress bar every second
	if (previous == current) return 1;
	previous = current;
	logprintf(".");
	return 1;
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

// Sends a string in a WebSocket frame to the server
static void writeWebSocketFrame(const char* format, ...) {
	// Gets length of formated payload
	va_list args;
	va_start(args, format);
	const uint32_t payloadLen = vsnprintf(NULL, 0, format, args);

	// Offset from start of frame to start of payload data
	uint8_t payloadOffset;

	// Creates websocket frame to send payload with
	char frame[8 + payloadLen + 1];

	// Sets fin bit, reserved bits and opcode for websocket frame
	frame[0] = 0x81;

	// Sets payload length and mask bit for websocket frame
	if (payloadLen >= WS_PAYLOADLEN_EXTENDED) {
		frame[1] = 0x80 | WS_PAYLOADLEN_EXTENDED;
		frame[2] = payloadLen >> 8;
		frame[3] = payloadLen;
		payloadOffset = 4;
	}
	else {
		frame[1] = 0x80 | payloadLen;
		payloadOffset = 2;
	}

	// Generates mask and inserts it into websocket frame
	const char mask[] = {rand() % 256, rand() % 256, rand() % 256, rand() % 256};
	memcpy(&frame[payloadOffset], mask, 4);
	payloadOffset += 4;

	// Inserts formated payload unmasked into frame
	va_start(args, format);
	vsprintf(frame + payloadOffset, format, args);
	va_end(args);

	// Maskes payload
	for (uint32_t i = 0; i < payloadLen; i++) {
		frame[i + payloadOffset] = frame[i + payloadOffset] ^ mask[i & 3];
	}

	// Sends frame
	verbaleyes_socket_write(frame, payloadLen + payloadOffset);
}



// Structure used to read and write configurable data
static struct confItem {
	char* name;
	uint8_t resetState;
	uint8_t len;
	uint16_t addr;
	bool nameMatchFailed;
};

// All configurable properties
static struct confItem conf_ssid = 			{ 	"ssid", 		0,		32, 	0 		};
static struct confItem conf_ssidkey = 		{	"ssidkey", 		0,		32, 	32 		};
static struct confItem conf_host = 			{ 	"host", 		2,		64, 	64 		};
static struct confItem conf_port = 			{ 	"port", 		2,		0,	 	128 	};
static struct confItem conf_path = 			{ 	"path", 		2,		32,	 	130 	};
static struct confItem conf_proj = 			{ 	"proj", 		2,		32,	 	162 	};
static struct confItem conf_projkey = 		{ 	"projkey", 		2,		32,	 	194 	};
static struct confItem conf_speedmin = 		{ 	"speedmin", 	10,		0,	 	226 	}; // Minimum scroll speed signed int
static struct confItem conf_speedmax = 		{ 	"speedmax", 	10,		0,	 	228 	}; // Maximum scroll speed signed int
static struct confItem conf_deadzone = 		{ 	"deadzone", 	10,		0,	 	230 	}; // Deadzone around 0 mark in % of entire range
static struct confItem conf_callow = 		{ 	"callow", 		10,		0,	 	232 	}; // Smallest value of analog read unsigned int
static struct confItem conf_calhigh = 		{ 	"calhigh", 		10,		0,	 	234 	}; // Largest value of analog read unsigned int
static struct confItem conf_sensitivity = 	{ 	"sensitivity", 	10,		0,	 	236 	}; // Sensitivity of analog read in % of entire range



// Reads a config value into a char array
static void confGetStr(struct confItem item, char* str) {
	for (uint8_t i = 0; i < item.len; i++) {
		str[i] = verbaleyes_conf_read(item.addr + i);
		if (str[i] == '\0') return;
	}
	str[item.len] = '\0';
}

// Reads a config value as a 2 byte int
static uint16_t confGetInt(struct confItem item) {
	return (verbaleyes_conf_read(item.addr) << 8) + verbaleyes_conf_read(item.addr + 1);
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

#define CONFFAILED 255

#define NOTHANDLING 0
#define HANDLINGKEY 1
#define HANDLINGVALUE 2

// Updates a configurable property from a stream of characters
bool updateConfig(const int16_t c) {
	static struct confItem* item;
	static uint8_t confState = 0;
	static uint8_t confIndex = 0;
	static uint16_t confBuffer = 0;
	static bool confBufferSigned = 0;

	switch (c) {
		// Updates configurable value
		default: {
			// Validates incomming key against valid configuration keys
			if (confState != HANDLINGVALUE) {
				// Initialize new configuration update
				if (confIndex == 0) {
					timeout = time(NULL) + CONFIGTIMEOUT;
					confState = HANDLINGKEY;
					logprintf("\n[ ");
				}
				// Prevents handling value for key with no match
				else if (confIndex == CONFFAILED) {
					return 1;
				}

				// Invalidates keys that does not match incomming string
				if (c != '=') {
					for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
						if (!confItems[i]->nameMatchFailed && c != confItems[i]->name[confIndex]) confItems[i]->nameMatchFailed = 1;
					}
					confIndex++;
					logprintf("%c", c);
				}
				// Finish matching key on delimiter input and setup to update its value
				else {
					// Finish matching key and reset all items
					for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
						if (confState == HANDLINGKEY && !confItems[i]->nameMatchFailed && confItems[i]->name[confIndex] == '\0') {
							item = confItems[i];
							confState = HANDLINGVALUE;
							logprintf(" ] is now: ");
						}

						confItems[i]->nameMatchFailed = 0;
					}

					// Prevents handling value for keys with no match
					if (confState == HANDLINGKEY) {
						confIndex = CONFFAILED;
						logprintf(" ] No matching key");
						return 1;
					}

					// Resets index to be reused for value
					confIndex = 0;
				}
			}
			// Updates string value for matched key
			else if (confIndex < item->len) {
				verbaleyes_conf_write(item->addr + confIndex, c);
				confIndex++;
				logprintf("%c", c);
			}
			// Updates integer value for matched key
			else if (item->len == 0 && confIndex < 5) {
				// Only accepts a valid numerical representation
				if (c >= '0' && c <= '9') {
					// Prints valid character
					logprintf("%c", c);

					// Clamps numbers higher than maximum value for an unsigned shorts
					if (confBuffer > 6553 || (confBuffer == 6553 && c > '5')) {
						logprintf("\nValue was too high, claming down to maximum value of 65535");
						confBuffer = 65535;
					}
					// Pushes number onto the buffer
					else {
						confBuffer = confBuffer * 10 + c - '0';
					}

					// Increments index
					confIndex++;
				}
				// Initialises int buffer to be signed
				else if (c == '-' && confIndex == 0) {
					confBufferSigned = 1;
					logprintf("-");
				}
				// Displays invalid character warning message once
				else {
					confIndex = CONFFAILED;
					logprintf("%c\nCharacter was not a number", c);
				}
			}
			// Displays max value length warning message once
			else if (confIndex != CONFFAILED) {
				confIndex = CONFFAILED;
				logprintf("\nMaximum input length reached");
			}

			// Continue reading more data
			return 1;
		}
		// Exits on no data read
		case '\0':
		case EOF: {
			// Exit if configuration is not actively being updated
			if (confState == NOTHANDLING) return 0;

			// Continues waiting for new data until timeout is reached
			if (time(NULL) < timeout) return 1;
		}
		// Terminates updating configurable data
		case '\n': {
			// Handles termination of value
			if (confState == HANDLINGVALUE) {
				// Terminates stored string
				if (item->len != 0) {
					if (confIndex < item->len) verbaleyes_conf_write(item->addr + confIndex, '\0');
				}
				// Stores 16 bit integer value
				else {
					// Special handling for negative numbers
					if (confBufferSigned) {
						// Clamps negative numbers lower than minimum value for a signed shorts
						if (confBuffer > 32767) {
							logprintf("\nValue was too low, claming up to minimum value of -32767");
							confBuffer = 65535;
						}
						// Makes buffer negative if it input string started with a minus sign
						else {
							confBuffer *= -1;
							confBufferSigned = 0;
						}
					}

					// Stores 16 bit integer for configuration item
					verbaleyes_conf_write(item->addr, confBuffer >> 8);
					verbaleyes_conf_write(item->addr + 1, confBuffer);
					confBuffer = 0;
				}

				// Indicates that value has been stored in configuration
				logprintf("\nValue was updated");

				// Pulls back state to handle updated value
				if (state > item->resetState) state = item->resetState;

				// Resets to handle new keys
				confState = HANDLINGKEY;
				confIndex = 0;
			}
			// Handles termination of key
			else if (confState == HANDLINGKEY) {
				// Commits all changed values and exits configuration handling
				if (confIndex == 0) {
					verbaleyes_conf_commit();
					logprintf("\nDone\n");
					confState = NOTHANDLING;
				}
				// Handles termination of value for key without a match
				else if (confIndex == CONFFAILED) {
					confIndex = 0;
				}
				// Handles termination before key was validated
				else {
					for (int8_t i = CONFITEMSLEN - 1; i >= 0; i--) {
						confItems[i]->nameMatchFailed = 0;
					}
					logprintf(" ] Aborted");
					confIndex = 0;
				}
			}

			return 1;
		}
	}
}



// Global variables for mapping analog scroll input
int16_t speedMin;
uint16_t speedCalLow;
float speedMapper;
float deadzoneSize;
float jitterSize;

#define RESINDEXFAILED 65535

// Ensures everything is connected to be able to transmit speed changes to the server
int8_t ensureConnection() {
	static uint16_t resIndex = 0;
	static uint8_t resMatchIndexes[5];
	static char acceptHeader[22 + 28 + 2] = "sec-websocket-accept: ";
	static char* host;
	static char* path;

	// Prevents immediately retrying after something fails
	if (state & 0x80) {
		if (timeout > time(NULL)) return CONNECTING;
		state &= 0x7F;
	}

	// Ensure network connection
	switch (state) {
		// Reconnects to network if connection is lost
		default: {
			if (verbaleyes_network_connected()) break;
			logprintf("\nLost connection to network");
		}
		// Initialize network connection
		case 0: {
			// Gets network ssid and key from config
			char ssid[conf_ssid.len + 1];
			confGetStr(conf_ssid, ssid);
			char ssidkey[conf_ssidkey.len + 1];
			confGetStr(conf_ssidkey, ssidkey);

			// Prints
			logprintf("\nConnecting to SSID: %s...", ssid);

			// Connects to ssid with key
			verbaleyes_network_connect(ssid, ssidkey);

			// Sets timeout for awaiting connection
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			state = 1;
		}
		// Completes network connection
		case 1: {
			// Awaits network connection established
			if (!verbaleyes_network_connected()) {
				// Shows progress bar until network is connected
				if (showProgressBar()) return CONNECTING;

				// Handles timeout error
				logprintf("\nFailed to connect to network");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 0 | 0x80;
				return CONNECTIONFAILED;
			}

			// Prints devices IP address
			const uint32_t ip = verbaleyes_network_getip();
			logprintf("\nIP address: %u.%u.%u.%u", (uint8_t)(ip), (uint8_t)(ip >> 8), (uint8_t)(ip >> 16), (uint8_t)(ip >> 24));
			state = 2;
		}
	}

	// Ensures connection to socket
	switch (state) {
		// Reconnects to socket if connection is lost
		default: {
			if (verbaleyes_socket_connected()) break;
			logprintf("\nLost connection to host");
		}
		// Initialize socket connection
		case 2: {
			// Gets host and port from config
			host = (char*)malloc(conf_host.len + 1);
			confGetStr(conf_host, host);
			uint16_t port = confGetInt(conf_port);
			path = (char*)malloc(conf_path.addr + 1);
			confGetStr(conf_path, path);

			// Prints
			logprintf("\nConnecting to server: %s:%hu%s...", host, port, path);

			// Connects to socket at host
			verbaleyes_socket_connect(host, port);

			// Sets timeout for awaiting connection
			timeout = time(NULL) + CONNECTINGTIMEOUT;
			state = 3;
		}
		// Completes socket connection
		case 3: {
			// Awaits socket connectin established
			if (!verbaleyes_socket_connected()) {
				// Shows progress bar until socket is connected
				if (showProgressBar()) return CONNECTING;

				// Handles timeout error
				logprintf("\nFailed to connect to host");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}
		}
		// Sends http request to use websocket protocol
		case 4: {
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

			// Sends HTTP request to setup WebSocket connection with host
			char req[4 + strlen(path) + 98 + 24 + 4 + 1];
			uint8_t reqlen = sprintf(
				req,
				"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: %s\r\n\r\n",
				path,
				host,
				key
			);
			verbaleyes_socket_write(req, reqlen);
			free(host);
			free(path);

			// Creates websocket accept header to compare against
			br_sha1_context ctx;
			br_sha1_init(&ctx);
			br_sha1_update(&ctx, key, 24);
			br_sha1_update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
			uint8_t hash[20];
			br_sha1_out(&ctx, hash);
			base64_encode_chars((const char*)hash, 20, acceptHeader + 22);
			strcpy(acceptHeader + 50, "\r\n");

			// Sets timeout value for awaiting http response
			timeout = time(NULL) + CONNECTINGTIMEOUT;

			// Sets up to read and verify http response
			resIndex = 0;
			state = 5;
		}
		// Validates HTTP status-line
		case 5: {
			while (resIndex != 12) {
				const int16_t c = verbaleyes_socket_read();

				// Shows progress bar until socket starts receiving data
				if (c == EOF) {
					if (resIndex == RESINDEXFAILED) {
						logprintf("\nReceived unexpected HTTP response code");
					}
					else if (!verbaleyes_socket_connected()) {
						logprintf("\nConnection to host closed");
					}
					else if (resIndex == 0) {
						if (showProgressBar()) return CONNECTING;
						logprintf("\nDid not get a response from server");
					}
					else {
						if (time(NULL) < timeout) return CONNECTING;
						logprintf("\nResponse from server ended prematurely");
					}

					timeout = time(NULL) + CONNECTIONFAILEDDELAY;
					state = 2 | 0x80;
					return CONNECTIONFAILED;
				}

				// Prints HTTP status-line
				if (c == '\n') {
					logprintf("\n%s", TAB);
				}
				else if (resIndex == 0) {
					logprintf("\n%s%c", TAB, c);
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
				// Failed to validate http status-line
				else {
					resIndex = RESINDEXFAILED;
				}
			}

			// Successfully validated status-line and sets up to validate http headers
			memset(resMatchIndexes, 0, sizeof resMatchIndexes);
			state = 6;
		}
		// Validates HTTP headers
		case 6: {
			while (resIndex != 4) {
				const int16_t c = verbaleyes_socket_read();

				// Handles timeout and socket close error
				if (c == EOF) {
					if (verbaleyes_socket_connected()) {
						if (time(NULL) < timeout) return CONNECTING;
						logprintf("\nResponse from server ended prematurely");
					}
					else {
						logprintf("\nConnection to host closed");
					}
					timeout = time(NULL) + CONNECTIONFAILEDDELAY;
					state = 2 | 0x80;
					return CONNECTIONFAILED;
				}

				// Analyzes HTTP headers up to end of head
				matchStr(&resIndex, c, "\r\n\r\n");

				// Prints HTTP headers
				if (c == '\n') {
					logprintf("\n%s", TAB);
				}
				else {
					logprintf("%c", c);
				}

				// Matches the incoming HTTP response against required and illigal substrings
				const char lowerc = tolower(c);
				matchStr(&resMatchIndexes[0], lowerc, "connection: upgrade\r\n");
				matchStr(&resMatchIndexes[1], lowerc, "upgrade: websocket\r\n");
				matchStr(&resMatchIndexes[2], (resMatchIndexes[2] <= 20) ? lowerc : c, acceptHeader);
				matchStr(&resMatchIndexes[3], lowerc, "sec-websocket-extensions: ");
				matchStr(&resMatchIndexes[4], lowerc, "sec-webSocket-protocol: ");
			}

			// Requires "Connection" header with "Upgrade" value and "Upgrade" header with "websocket" value
			if (!resMatchIndexes[0] || !resMatchIndexes[1]) {
				logprintf("\nHTTP response is not an upgrade to the WebSockets protocol");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}
			// Requires WebSocket accept header with correct value
			else if (!resMatchIndexes[2]) {
				logprintf("\nMissing or incorrect WebSocket accept header");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}
			// Checks for non-requested WebSocket extension header
			else if (resMatchIndexes[3]) {
				logprintf("\nUnexpected WebSocket Extension header");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}
			// Checks for non-requested WebSocket protocol header
			else if (resMatchIndexes[4]) {
				logprintf("\nUnexpected WebSocket Protocol header");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}

			// Successfully validated http headers
			logprintf("\nWebSocket connection established");

			// Sets up to connect to project
			state = 7;
		}
	}

	// Connects to verbalEyes project
	switch (state) {
		// Connect to verbalEyes project
		case 7: {
			// Gets project and project key from config
			char proj[conf_proj.len + 1];
			confGetStr(conf_proj, proj);
			char projkey[conf_projkey.len + 1];
			confGetStr(conf_projkey, projkey);

			// Prints
			logprintf("\nConnecting to project: %s...", proj);

			// Sends VerbalEyes project authentication request
			writeWebSocketFrame("{\"_core\": {\"auth\": {\"id\": \"%s\", \"key\": \"%s\"}}}", proj, projkey);

			// Sets timeout value for awaiting websocket response
			timeout = time(NULL) + CONNECTINGTIMEOUT;

			// Sets up to read and verify websocket response
			resIndex = 0;
			state = 8;
		}
		// Validates WebSocket opcode for authentication
		case 8: {
			const int16_t c = verbaleyes_socket_read();

			// Shows progress bar until socket starts receiving data
			if (c == EOF) {
				if (resIndex == 0) {
					if (showProgressBar()) return 1;
					logprintf("\nDid not get a response from the server");
				}
				else {
					if (time(NULL) < timeout) return 1;
					logprintf("\nResponse from server ended prematurely");
				}

				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}

			// Makes sure this is an unfragmented WebSocket frame in text format
			if (c != 0x81) {
				logprintf("\nReceived response data is either not a WebSocket frame or uses an unsupported WebSocket feature");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}

			// Sets up to read WebSocket payload length
			resIndex = WS_PAYLOADLEN_NOTSET;
			state = 9;
		}
		// Gets length of WebSocket payload for authentication
		case 9: {
			while (1) {
				const uint16_t c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) {
					if (time(NULL) < timeout) return 1;
					logprintf("\nResponse from server ended prematurely");
					timeout = time(NULL) + CONNECTIONFAILEDDELAY;
					state = 2 | 0x80;
					return CONNECTIONFAILED;
				}

				// Gets payload length and continues if extended payload length is used
				if (resIndex == WS_PAYLOADLEN_NOTSET) {
					// Server is not allowed to mask messages sent to the client according to the spec
					if (c & 0x80) {
						logprintf("\nReveiced a masked frame which is not allowed");
						timeout = time(NULL) + CONNECTIONFAILEDDELAY;
						state = 2 | 0x80;
						return CONNECTIONFAILED;
					}

					// Gets payload length without mask bit
					resIndex = c & 0x7F;

					// Moves on if payload length was defined in one byte
					if (resIndex < WS_PAYLOADLEN_EXTENDED) break;

					// Aborts if payload length requires more than the 16 bits available in resIndex
					if (resIndex == 127) {
						logprintf("\nWebsocket frame was unexpectedly long");
						timeout = time(NULL) + CONNECTIONFAILEDDELAY;
						state = 2 | 0x80;
						return CONNECTIONFAILED;
					}
				}
				// Gets first byte of extended payload length
				else if (resIndex == WS_PAYLOADLEN_EXTENDED) {
					resIndex = c << 8;
				}
				// Adds second byte of extended payload length and moves on
				else {
					resIndex += c;
					break;
				}
			}

			// Sets up to read WebSocket payload
			logprintf("\nReceived authentication response:\n%s", TAB);
			resMatchIndexes[0] = 0;
			state = 10;
		}
		// Validates WebSocket payload for authentication
		case 10: {
			// Reads entire WebSocket authentication response
			while (resIndex) {
				const signed short c = verbaleyes_socket_read();

				// Handles timeout error
				if (c == EOF) {
					if (time(NULL) < timeout) return 1;
					logprintf("\nResponse from server ended prematurely");
					timeout = time(NULL) + CONNECTIONFAILEDDELAY;
					state = 2 | 0x80;
					return CONNECTIONFAILED;
				}

				// Prints entire WebSocket payload
				if (c == '\n') {
					logprintf("\n%s", TAB);
				}
				else {
					logprintf("%c", c);
				}

				// Makes sure authentication was successful
				if (resMatchIndexes[0] != SUCCESSFULMATCH) matchStr(&resMatchIndexes[0], c, "authed");
				resIndex--;
			}

			// Validates authentication
			if (!resMatchIndexes[0]) {
				logprintf("\nAuthentication failed");
				timeout = time(NULL) + CONNECTIONFAILEDDELAY;
				state = 2 | 0x80;
				return CONNECTIONFAILED;
			}

			// Moves on for successful authentication
			logprintf("\nAuthenticated");
			state = 11;
		}
		// Sets global values used for updating speed
		case 11: {
			// Gets deadzone an sensitivity percentage values from config
			const float deadzone = confGetInt(conf_deadzone);
			const float sensitivity = confGetInt(conf_sensitivity);

			// Gets minimum and maximum speed from config
			speedMin = confGetInt(conf_speedmin);
			const int16_t speedMax = confGetInt(conf_speedmax);

			// Gets calibration start and end point to use on analog read value
			speedCalLow = confGetInt(conf_callow);
			const uint16_t speedCalHigh = confGetInt(conf_calhigh);

			// Sets helper values to use when mapping analog read value to new range
			const float speedSize = (speedMax - speedMin) / (1 - deadzone / 100);
			speedMapper = speedSize / (speedCalHigh - speedCalLow);
			deadzoneSize = speedSize * deadzone / 100;
			jitterSize = speedSize * sensitivity / 100;

			// Prints settings
			logprintf(
				"\nSetting up speed reader with:\n\tMinimum speed at: %i\n\tMaximum speed at: %i\n\tDeadzone at: %.0f%%\n\tSensitivity at: %.0f%%\n\tCalibration low at: %u\n\tCalibration high: %u\n",
				speedMin,
				speedMax,
				deadzone,
				sensitivity,
				speedCalLow,
				speedCalHigh
			);

			// Sets state bo be outside range now that it is done
			state = 12;
		}
	}

	// Allows caller function to continue past this function
	return CONNECTED;
}

// Sends remapped analog speed reading to the server
void updateSpeed(const uint16_t value) {
	static float speed;

	// Maps analog input value to conf range
	float mappedValue = (float)(value - speedCalLow) * speedMapper + speedMin;

	// Shifts mapped value above deadzone
	if (mappedValue > deadzoneSize) {
		mappedValue -= deadzoneSize;
	}
	// Clamps value to deadzone around 0 mark
	else if (mappedValue > 0) {
		if (speed == 0) return;
		mappedValue = 0;
	}

	// Supresses updating speed if it has not changed enough unless it is updated to zero
	if (mappedValue != 0 && mappedValue <= speed + jitterSize && mappedValue >= speed - jitterSize ) return;
	speed = mappedValue;

	// Sends new speed to the server
	writeWebSocketFrame("{\"_core\": {\"doc\": {\"id\": \"test\", \"speed\": %.2f}}}", speed);

	// Prints new speed
	logprintf("\nSpeed has been updated to: %.2f", speed);
}

// Tells server to reset position to 0 when button is pressed
void jumpToTop(const bool value) {
	static bool state = 1;

	// Only sends data on button down event
	if (value == state) return;
	state = value;
	if (value == 0) return;

	// Sends message to server
	writeWebSocketFrame("{\"_core\": {\"doc\": {\"id\": \"test\", \"offset\": 0}}}");

	// Prints
	logprintf("\nScroll position has been set to: 0");
}
