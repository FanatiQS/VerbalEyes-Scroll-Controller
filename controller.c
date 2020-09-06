#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <bearssl/bearssl_hash.h> // sha1
#include <libb64/cencode.h> // base64

#include "definitions.h"
#include "conf_items.h"



// Global variables for mapping analog scroll input
short speedCalLow;
signed short speedMin;
float speedMapper;
float deadzoneSize;
float jitterSize;

// Max speed the analog read can supply
int POT_MAX = 1024;



// Context structure for parsing incomming data when updating configuration values
struct config_ctx {
	short addr;
	short len;
	short index;
	short intBuffer;
	bool states[confListLength];
};

// Default values for configuration context
const struct config_ctx init_ctx = {-1};

// Matches and validates incoming keys
void confMatchKey(struct config_ctx *ctx, const char c) {
	// Completes key matching/validation
	if (c == '=') {
		// Looks for valid conf items with terminated strings to find matching key
		for (int i = confListLength - 1; i >= 0; i--) {
			if (ctx->states[i] == 0 && confList[i]->name[ctx->index] == '\0') {
				serialWriteString(" ] is now: ");
				ctx->addr = confList[i]->addr;
				ctx->len = confList[i]->len;
				ctx->index = 0;
				return;
			}
		}

		//!! prints
		serialWriteString(" ] No matching key");

		// Prevents handling value by making sure it is in value handle state but with a length that is shorter than the index
		ctx->addr = 0;
		ctx->len = -1;
	}
	// Checks validation for incomming character against all conf items
	else {
		// Invalidates conf items that did not match incomming character
		for (int i = confListLength - 1; i >= 0; i--) {
			if (ctx->states[i] == 0 && confList[i]->name[ctx->index] != c) {
				ctx->states[i] = 1;
			}
		}

		//!! prints
		serialWriteChar(c);

		// Prepair for next handling next character
		ctx->index++;
	}
}

// Adds value to EEPROM until max length is reached
void confUpdateValue(struct config_ctx *ctx, const char c) {
	// Buffers character if it is of type int and max value length is not reached
	if (!ctx->len && ctx->index <= 6) {
		// Only accepts a valid numerical representation
		if (c >= '0' && c <= '9') {
			// Pushes character onto int buffer if it is unsigned and within range
			if (ctx->intBuffer >= 0) {
				ctx->intBuffer = ctx->intBuffer * 10 + c - '0';
			}
			// Pushes character onto int buffer if it is unsigned
			else {
				ctx->intBuffer = ctx->intBuffer * 10 + '0' - c;
			}

			//!! prints
			serialWriteChar(c);
		}
		// Initialises int buffer to be signed
		else if (c == '-' && ctx->index == 0) {
			ctx->intBuffer = -32768;
			serialWriteChar('-');
		}
		// Displays invalid character warning message once
		else {
			ctx->index = 7;
			serialWriteString("\nCharacter was not a number");
			return;
		}
	}
	// Stores the character in EEPROM if it is a string and max value length is not reached
	else if (ctx->index < ctx->len) {
		serialWriteChar(c);
		confWrite(ctx->addr + ctx->index, c);
	}
	// Displays max value length warning message once
	else {
		if (ctx->index != ctx->len) return;
		serialWriteString("\nMaximum input length reached");
	}
	ctx->index++;
}

// Parses incomming data and updates matching conf items EEPROM value if key existed in "confList"
void initialize();
void confParse(struct config_ctx *ctx, bool *touched, const char c) {
	// Wraps up parsing of line
	if (c == '\n') {
		// Finalises writing of data
		if (ctx->addr != -1) {
			// Stores buffered int in 2 bytes
			if (!ctx->len) {
				if (ctx->intBuffer > 65535 || ctx->intBuffer < -32767) {
					serialWriteString("\nNumber is outside range");
				}
				else {
					confWrite(ctx->addr, ctx->intBuffer >> 8);
					confWrite(ctx->addr + 1, ctx->intBuffer & 0xff);
				}
			}
			// Terminates string if max length was not reached
			else if (ctx->index < ctx->len) confWrite(ctx->addr + ctx->index, '\0');
		}
		// Message did not contain a delimiter character
		else if (ctx->index != 0) {
			serialWriteString(" ] Aborted");
		}
		// Incoming message was empty
		else {
			// Commits changes to EEPROM and reconnect everything if context has been touched
			if (*touched == 1) {
				*touched = 0;
				confCommit();
				serialWriteString("Done. Restarting...\n\n");
				initialize();
			}
			//!! prints
			else {
				serialWriteString("Received no input data to store\n");
			}

			// No need to reset the context since it has not been altered
			return;
		}

		//!! prints
		serialWriteChar('\n');

		// Resets context
		*ctx = init_ctx;
	}
	// Validates key from message
	else if (ctx->addr == -1) {
		// Marks context as touched to precent untoched contexts from printing "Done"
		if (ctx->index == 0) {
			*touched = 1;
			serialWriteString("[ ");
		}

		// Checks key validation and sets up for value handling on key match
		confMatchKey(ctx, c);
	}
	// Stores message value in EEPROM
	else {
		confUpdateValue(ctx, c);
	}
}

// Updates conf items from data received over serial
//!! I'm not super proud of this function, want to find a better way of writing it, but it works
void confSerialLoop() {
	struct config_ctx ctx = init_ctx;
	bool touched = 0;

	//!! prints
	serialWriteChar('\n');

	// Parse all incomming characters
	while (1) {
		yield();

		// Parses incomming character
		confParse(&ctx, &touched, serialRead());

		// Closes parser if there is no more data
		if (!serialHasData()) {
			// Checks if parser was already closed
			if (touched == 0) return;

			// Awaits more data over serial for maximum 500ms before forcing parser to close
			const unsigned long timeout = getTime() + 500;
			while (!serialHasData()) {
				yield();
				if (timeout < getTime()) {
					if (ctx.addr != -1 || ctx.index != 0) confParse(&ctx, &touched, '\n');
					confParse(&ctx, &touched, '\n');
					return;
				}
			}
		}
	}
}

// Gets string value from EEPROM address range
void confGetString(struct config conf, char value[]) {
	for (int i = 0; i < conf.len; i++) {
		value[i] = confRead(conf.addr + i);
		if (value[i] == '\0') return;
	}
	value[conf.len] = '\0';
}

// Gets 2 byte int value from EEPROM address
int confGetInt(struct config conf) {
	return (confRead(conf.addr) << 8) + confRead(conf.addr + 1);
}



// Displays a progress bar and enables handling conf input over serial
void progressbar() {
	static unsigned long timeout = 0;
	yield();

	// Shows progress indicator every 200 milliseconds
	const unsigned long current = getTime();
	if (timeout < current) {
		timeout = current + 200;
		serialWriteChar('.');
	}

	// Handles incoming serial data during progression
	if (serialHasData()) {
		serialWriteChar('\n');
		confSerialLoop();
		serialWriteString("continuing...");
	}
}

// Writes an int as a string to the serial interface
void serialWriteInt(const int value) {
	char str[6];
	sprintf(str, "%i", value);
	serialWriteString(str);
}

// Writes a character to the serial interface
void serialWriteChar(const char c) {
	const char str[2] = { c, '\0' };
	serialWriteString(str);
}

// Writes a string to the serial interface
void serialWriteString(const char str[]) {
	serialWrite(str);
}



// Size to use for indentation
const char tab[] = "    ";

// Wait for response data from host
bool socketWaitForResponse() {
	const unsigned long timeout = getTime() + 10000;
	while (!socketHasData()) {
		if (timeout < getTime()) return 0;
		progressbar();
	}
	return 1;
}

// Matches incoming message character by character
void socketMatchData(int *i, const char c, const char match[]) {
	if (*i == -1) return;
	if (match[*i] == c) *i += 1;
	else if (match[*i] == '\0') *i = -1;
	else *i = 0;
}

// Sends a string in a WebSocket frame to the server
void writeWebSocketFrame(char str[]) {
	// Generates mask
	const char mask[] = {rand() % 256, rand() % 256, rand() % 256, rand() % 256};

	// Gets offset and length for payload
	const int len = strlen(str);
	const int offset = (len > 125) ? 8 : 6;

	// Masks and shifts payload
	for (int i = len - 1; i >= 0; i--) {
		str[i + offset] = str[i] ^ mask[i & 3];
	}

	// Sets first byte
	str[0] = 0x81;

	// Sets payload length bytes and mask bit
	if (offset == 8) {
		str[1] = 254;
		str[2] = len >> 8;
		str[3] = len;
  }
	else {
		str[1] = 0x80 | len;
	}

	// Appends mask
	str[offset - 4] = mask[0];
	str[offset - 3] = mask[1];
	str[offset - 2] = mask[2];
	str[offset - 1] = mask[3];

	// Sends frame
	socketWrite(str, len + offset);
}



// Connects to the wifi network stored in EEPROM
void connectNetwork() {
	// Gets ssid and ssidKey from EEPROM
	char ssid[conf_ssid.len + 1];
	confGetString(conf_ssid, ssid);
	char ssidKey[conf_ssidKey.len + 1];
	confGetString(conf_ssidKey, ssidKey);

	//!! prints
	serialWriteString("Connecting to SSID: ");
	serialWriteString(ssid);
	serialWriteString("...");

	// Connects to the wifi network
	networkConnect(ssid, ssidKey);

	// Awaits established wifi connection
//	int brightness = 0; //!! used for testing led indication
	while (!networkStatus()) {
		progressbar();
		//!! Testing led indication
//		if (brightness > 1000) brightness = -1000;
//		brightness += 5;
//		analogWrite(LED_BUILTIN, abs(brightness));
//		delay(10);
	}
	serialWriteChar('\n');

	//!! prints
	serialWriteString("IP address: ");
	//!! Serial.print(WiFi.localIP());
	serialWriteChar('\n');
}

// Connects to the host, establishes a WebSocket connection and sends a VerbalEyes authentication request
void connectSocket() {
	// Gets host from EEPROM
	char host[conf_host.len + 1];
	confGetString(conf_host, host);

	// Gets port from EEPROM
	unsigned short port = confGetInt(conf_port);

	//!! prints
	serialWriteString("Connecting to Host: ");
	serialWriteString(host);
	serialWriteChar(':');
	serialWriteInt(port);

	// Connects to host and retries if connection fails
	if (!socketConnect(host, port)) {
		serialWriteString("\nConnection failed\n");
		return;
	}

	// Sets random seed
	srand(randomSeed());

	// Generates websocket key
	const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char key[24];
	for (short i = 0; i < 21; i++) {
		key[i] = table[rand() % 64];
	}
	key[21] = table[rand() % 4 * 16];
	key[22] = '=';
	key[23] = '=';

	// Sends HTTP request to setup WebSocket connection with host
	socketWrite("GET / HTTP/1.1\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: ", 103);
	socketWrite(key, 24);
	socketWrite("\r\nHost: ", 8);
	socketWrite(host, strlen(host));
	socketWrite("\r\n\r\n", 4);

	// Creates websocket accept header to compare against
	uint8_t hash[20];
	br_sha1_context ctx;
	br_sha1_init(&ctx);
	br_sha1_update(&ctx, key, 24);
	br_sha1_update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
	br_sha1_out(&ctx, hash);
	char accept[22 + 28 + 2] = "Sec-WebSocket-Accept: ";
	base64_encode_chars((const char*) hash, 20, accept + 22);
	sprintf(accept + 50, "\r\n");

	// Awaits HTTP response
	serialWriteString("...");
	if (!socketWaitForResponse()) {
		serialWriteString("\nTimed out waiting for HTTP response\n");
		socketClose();
		return;
	}
	serialWriteChar('\n');

	//!! prints
	serialWriteString("Received HTTP response:\n");
	serialWriteString(tab);

	// Matches the incoming HTTP response against required and illigal substrings
	bool EOL = 0;
	int matches[6] = {};
	while (socketHasData()) {
		yield();
		const char c = socketRead();
		if (c == '\n') EOL = 1;
		else if (c != '\r') {
			if (EOL == 1) {
				serialWriteChar('\n');
				serialWriteString(tab);
				EOL = 0;
			}
			serialWriteChar(c);
		}
		socketMatchData(&matches[0], tolower(c), "http/1.1 101");
		socketMatchData(&matches[1], tolower(c), "connection: upgrade\r\n");
		socketMatchData(&matches[2], tolower(c), "upgrade: websocket\r\n");
		socketMatchData(&matches[3], c, accept);
		socketMatchData(&matches[4], c, "Sec-WebSocket-Extensions: ");
		socketMatchData(&matches[5], c, "Sec-WebSocket-Protocol: ");
	}
	serialWriteChar('\n');

	// Requires HTTP response code 101
	if (!matches[0]) {
		serialWriteString("Received unexpected HTTP response code\n");
		socketClose();
		return;
	}

	// Requires "Connection" header with "Upgrade" value and "Upgrade" header with "websocket" value
	if (!matches[1] || !matches[2]) {
		serialWriteString("HTTP response is not an upgrade to the WebSockets protocol\n");
		socketClose();
		return;
	}

	// Requires WebSocket accept header with correct value
	if (!matches[3]) {
		serialWriteString("Missing or incorrect WebSocket accept header\n");
		socketClose();
		return;
	}

	// Checks for non-requested WebSocket extension header
	if (matches[4]) {
		serialWriteString("Unexpected WebSocket Extension header\n");
		socketClose();
		return;
	}

	// Checks for non-requested WebSocket protocol header
	if (matches[5]) {
		serialWriteString("Unexpected WebSocket Protocol header\n");
		socketClose();
		return;
	}

	//!! prints
	serialWriteString("WebSocket connection established\n");

	// Gets project and project key from EEPROM
	char proj[conf_proj.len + 1];
	confGetString(conf_proj, proj);
	char projKey[conf_projKey.len + 1];
	confGetString(conf_projKey, projKey);

	//!! prints
	serialWriteString("Connecting to project: ");
	serialWriteString(proj);

	// Sends VerbalEyes project authentication request
	char auth[8 + 43 + conf_proj.len + conf_projKey.len];
	sprintf(auth, "{\"_core\": {\"auth\": {\"id\": \"%s\", \"key\": \"%s\"}}}", proj, projKey);
	writeWebSocketFrame(auth);

	// Await authentication response from server
	if (!socketWaitForResponse()) {
		serialWriteString("\nTimed out waiting for authentication response\n");
		socketClose();
		return;
	}
	serialWriteChar('\n');

	// Checks that this is an unfragmented WebSocket frame in text format
	if (socketRead() != 0x81) {
		serialWriteString("Received response data is either not a WebSocket frame or uses an unsupported WebSocket feature\n");
		socketClose();
		return;
	}

	// This byte contains the maskBit and the payload length
	const char byte2 = socketRead();

	// Server is not allowed to mask messages sent to the client according to the spec
	if (byte2 >= 0x80) {
		serialWriteString("Reveiced a masked frame which is not allowed\n");
		socketClose();
		return;
	}

	// Gets extended payload length if it is longer that 125
	int payloadLen = byte2 & 0x7F;
	if (payloadLen == 126) {
		payloadLen = socketRead() << 8;
		payloadLen = socketRead();
	}
	else if (payloadLen == 127) {
		serialWriteString("The received response data payload was too long\n");
		socketClose();
		return;
	}

	//!! prints
	serialWriteString("Received authentication response:\n");
	serialWriteString(tab);

	// Matches the incoming WebSocket string against required substring
	int authed = 0;
	for (int i = 0; i < payloadLen; i++) {
		yield();
		const char c = socketRead();
		serialWriteChar(c);
		socketMatchData(&authed, c, "\"authed\":");
	}
	serialWriteChar('\n');

	// Validates the authentication
	if (!authed) {
		serialWriteString("Authentication Failed\n");
		socketClose();
		return;
	}

	//!! prints
	serialWriteString("Authenticated\n\n");
}

// Connects to network and socket server and gets calibration
void initialize() {
	// Connects to the network and socket server using conf item values
	connectNetwork();
	connectSocket();

	// Retries to connect to socket server until succcess if it failed
	while (!socketStatus()) {
		serialWriteString("Retrying to connect to the host in 3 seconds\n");
		const unsigned long timeout = getTime() + 3000;
		while (timeout > getTime()) {
			yield();
			if (serialHasData()) confSerialLoop();
		}
		connectSocket();
	}

	// Sets global values for speed updating
	const float deadzone = confGetInt(conf_speedDeadzone);
	speedMin = confGetInt(conf_speedMin);
	const float speedSize = (float)(confGetInt(conf_speedMax) - speedMin) / (1 - deadzone / 100);
	speedCalLow = confGetInt(conf_calLow);
	speedMapper = speedSize / (POT_MAX - speedCalLow - confGetInt(conf_calHigh));
	deadzoneSize = speedSize * deadzone / 100;
	jitterSize = speedSize * confGetInt(conf_calIgnorejitter) / 100;
}



// Sends remapped analog speed reading to the server
void updateSpeed(const int value) {
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

	// Supresses updating speed if it has not changed enough
	if (mappedValue != 0 && mappedValue < speed + jitterSize && mappedValue > speed - jitterSize )
	{
		infoLED(1);
		return;
	}

	// Turns on info LED
	infoLED(0);
	speed = mappedValue;

	// Puts speed float in a string
	const int speedStrLen = (((int)speed == 0) ? 0 : log10(abs((int)floor(speed)))) + 5;
	char speedStr[speedStrLen];
	sprintf(speedStr, "%.2f", speed);

	// Sends speed update message to server
	char msg[8 + 32 + strlen(speed_str)];
	strcpy(msg, "{\"_core\": {\"doc\": {\"speed\": ");
	strcpy(msg + 28, speed_str);
	strcpy(msg + 28 + strlen(speed_str), "}}}");
	writeWebSocketFrame(msg);

	//!! prints
	serialWriteString("Speed has been updated to: ");
	serialWriteString(speedStr);
	serialWriteChar('\n');
}

// Tells server to reset position to 0 when button is pressed
void jumpToTop(int pin) {
	static bool state;

	// Only sends data on button down event
	const bool value = !readButton(pin);
	if (value == state) return;
	state = value;
	if (value == 0) return;

	// Sends message to server
	char msg[8 + 34] = "{\"_core\": {\"doc\": {\"offset\": 0}}}";
	writeWebSocketFrame(msg);

	//!! prints
	serialWriteString("Scroll position has been set to: 0\n");
}
