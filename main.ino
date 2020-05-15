/* Reading and Writing from the command line:
 * Get the tty path
 * OSX: ls /dev/cu.usbserial-*
 * Linux: ls /dev/ttyUSB*
 * Windows: ?
 *
 * Writing:
 * OSX/Linux: echo string > path
 * terminal ends message with a LF
 * if the string contains spaces, they have to be escaped or the string has to be in quotes
 * Windows: ?
 *
 * Reading:
 * OSX/Linux: cat path
 * if it does not continue to read, try to run "stty -echo -F path" (had this issue in raspbian)
 * Windows: ?
 */

/* TODO:
 * LED to indicate what is going on using the built in led during initilization and during setting conf data

 * Is it required to "subscribe" to a document after we have been authenticated? We want to subscribe to default document anyway.
   This would only be needed if the server doesn't have the default document open and can not understand it should open it. "{\"_core\": {\"sub\": \"\"}}"
   Right now it is not used, but it was used in old code and is here just in case I would realize it is actually needed.

 * Parsing: Support for escape character when writing configuration data over serial?   static int esc = 0;
 // Escapes this character
 if (esc == 1) {
 	esc = 0;
 }
 // Escapes next character
 else if (c == '\\') {
 	esc = 1;
 	return 0;
 }

 * Parsing: Support comments, ignore that entire line
 From old readme * Comments are supported with # sign. Everything between # and EOL is ignored


 * Parsing: Support spaces in start and end. Trim them out from both key validation, value writing to eeprom and writing to serial?
 From old readme, this is a guideline for how it could (and has previously been) implemented
 * Space trimming:
 *		All spaces before the key will be trimmed, therefore a key can not start with a space
 *		All spaces after the key will be trimmed unless there is a matching key, therefor a key can end with one or multiple spaces
 *		A key can contain spaces anywhere except at the start, but the end is safe
 *		The value only ever gets one space trimmed at the start
 * Old code:
 // Validates space character input for candidates
 if (c == ' ') {
	 for (int i = KEYSLAST; i >= 0; i--) {
		 if (valid[i] != 1) continue;
		 if (keys[i][index] == '\0') valid[i] = -1;
		 else if (keys[i][index] != c) valid[i] = 0;
	 }
	 spaces++;
 }
 // Validates other character matches for candidates
 else {
	 //!! print
	 while (spaces > 1) {
		 Serial.print(' ');
		 spaces--;
	 }
 }

 * Add support for partial data on socket. When reading socket data, currently the processing stops after the sockets data buffer is empty.
   Should add some kind of check that if it would fail the checks later, it will wait 500ms or so to then give it another shot.

 * Define bool type if it is not defined already?

 * Create a nice update system. Previous idea that was not liked by tech:
 	* Add look for updates on host. Admins can upload the .bin file to the server and the arduino code will check if it needs updating.
	But no central server updating to prevent introducing bugs without actively making the choice to update

 */

  /* NOTES FROM OLD FILE

   * Errors:
   *		Every key has a maximum length but everything up until the max length is reached will still be written to the EEPROM. This can result in incomplete and there for invalid data stored
   *		If a key is not found, the input will be ignored until it gets an EOL character. This is by design

   * EOL finnishes parsing and writing conf data. If data is incomplete, it discards current data and makes it possible to submit a new input. It can be inserted anywhere before delimiter to restart
   * Valid keys are anything that does not contain an "="
   * The syntax is similar to INI or ENV, so values are not put in quotes. Spaces in keys do not need to be escaped. In fact, escaping is not supported as of now.
   * INI sections are not supported
   * Keys are case sensitive
   * Syntax (not valid: comments and speces are not supported and ; is not EOL): # comments are ignored;[spaces >= 0][key][spaces >= 0]=[spaces 0-1][value]
    */

  /* LED INDICATION:
   * writing to conf

   * network connecting

   * port outside range			- bad host/port		-	configuration issue and should not occur with configuration software
   * host connection failed		- bad host/port		-	no server at host:port
   * http response timeout		- bad host/port		-	no responce from server
   * websocket response bad		- bad host/port		-	server does not know websocket
   * authentication timeout		- bad host/port		-	server did not send a websocket response
   * websocket format bad		- bad host/port		-	server fucked up websocket frame
   * websocket payload too long	- bad host/port		-	server sent a really long message

   * authentication failed		- bad proj/proj_key	-	no such project or wrong password

   * standby

   * bad host/port: There might not be a VerbalEyes server at host:port or the server is not acting propperly.
   */

#include <ESP8266WiFi.h>
//!! #include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <bearssl/bearssl_hash.h> // sha1
#include <libb64/cencode.h> // base64


//!!
void setup() {
	EEPROM.begin(512);
	Serial.begin(9600);
	Serial.print("\n\n\n");
	pinMode(LED_BUILTIN, OUTPUT);

	connect();
}


//!!
void loop() {
	if (serialHasData()) confSerialLoop();
}















/* Some Arduino/ESP functions used in the code that I want to be pure C
 * millis() // for timeouts
 * micros() // for setting random seed
 * yield() // for doing network stuff during loops
 */

// Connects to a WiFi network
void networkConnect(const char ssid[], const char key[]) {
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, key);
}

// Gets the connection status of the WiFi connection
bool networkStatus() {
	return (WiFi.status() == WL_CONNECTED);
}



// Creates a WiFi client to use for WebSocket communication with the server
WiFiClient client;

/* There is commented out alternative code in "socketConnect" and under here that should work for SSL, but there are two issues.
 * The first issue is that it requires a pre-defined fingerprint, that is not implemented in the conf system and sound like it would not be ituitive for end users.
 * The second issue is that "client.write()", used to send websocket and http data to the server, just does not like having a predefined length when using "WiFiClientSecure".
   I assume this is some kind of bug since it is supposed to be the same function, but with SSL. This is something that needs to be investigated more in the future.
 */
//WiFiClientSecure client; //!!## problem with client.write not liking having a defined size.
//const char fingerprint[] PROGMEM = "59 74 61 88 13 CA 12 34 15 4D 11 0A C1 7F E6 67 07 69 42 F5";

// Connects the socket to a server
bool socketConnect(const char host[], const short port) {
//  client.setFingerprint(fingerprint);
//	return client.connect("demos.kaazing.com/echo", 80);
	return client.connect(host, port);
}

// Gets the connection status of the socket connection
bool socketStatus() {
	return client.connected();
}

// Closes the open socket
void socketClose() {
	client.stop();
}

// Checks if the socket has data to read
bool socketHasData() {
	return (client.available() > 0);
}

// Consumes a single character from the sockets response data buffer
char socketRead() {
	return client.read();
}

// Sends a string to the server that the socket is connected to
void socketWrite(const char str[], const int len) {
	client.write(str, len);
}



// Checks if serial port has any data read
bool serialHasData() {
	return (Serial.available() > 0);
}

// Consumes a single character from serial buffer
char serialRead() {
	return Serial.read();
}

//!! Serial.print is still used. Mainly because it prints the ip in connectNetwork and that one is not a string, but also because currently it prints both strings and characters.
//!! In the future, this is the one that is supposed to be used instead of Serial.print.
void serialPrint(const char str[]) {
	Serial.print(str);
}



// Reads a character from address in EEPROM
char confRead(const int addr) {
	return EEPROM.read(addr);
}

// Writes character to address in EEPROM
void confWrite(const int addr, const char c) {
	EEPROM.write(addr, c);
}








// Structure for creating a conf item
struct config {
	int addr;
	int len;
	char *name;
};



// Creates config item for SSID
struct config conf_ssid = {
	0,
	32,
	"ssid"
};

// Creates config item for SSID key
struct config conf_ssidKey = {
	conf_ssid.addr + conf_ssid.len,
	63,
	"ssidKey"
};

// Creates config item for websocket server
struct config conf_host = {
	conf_ssidKey.addr + conf_ssidKey.len,
	64,
	"host"
};

// Creates config item for websocket servers port
struct config conf_port = {
	conf_host.addr + conf_host.len,
	5,
	"port"
};

// Creates config item for verbaleyes project name
struct config conf_proj = {
	conf_port.addr + conf_port.len,
	32,
	"proj"
};

// Creates config item for verbaleyes project password
struct config conf_projKey = {
	conf_proj.addr + conf_proj.len,
	64,
	"projKey"
};



// A list of all conf items
struct config confList[] = {
	conf_ssid,
	conf_ssidKey,
	conf_host,
	conf_port,
	conf_proj,
	conf_projKey
};

// Number of conf items
#define confListLength sizeof confList / sizeof confList[0]



// Context structure for parsing incomming data when updating configuration values
struct config_ctx {
	int addr;
	int len;
	int index;
	bool states[confListLength];
};

// Default values for configuration context
const config_ctx init_ctx = {-1};

// Matches and validates incoming keys
void confMatchKey(struct config_ctx *ctx, const char c) {
	// Completes key matching/validation
	if (c == '=') {
		// Looks for valid conf items with terminated strings to find matching key
		for (int i = confListLength - 1; i >= 0; i--) {
			if (ctx->states[i] == 0 && confList[i].name[ctx->index] == '\0') {
				Serial.print(" ] is now: ");
				ctx->addr = confList[i].addr;
				ctx->len = confList[i].len;
				ctx->index = 0;
				return;
			}
		}

		//!! prints
		Serial.print(" ] No matching key");

		// Prevents handling value by making sure it is in value handle state but with a length that is shorter than the index
		ctx->addr = 0;
		ctx->len = -1;
	}
	// Checks validation for incomming character against all conf items
	else {
		// Invalidates conf items that did not match incomming character
		for (int i = confListLength - 1; i >= 0; i--) {
			if (ctx->states[i] == 0 && confList[i].name[ctx->index] != c) {
				ctx->states[i] = 1;
			}
		}

		//!! prints
		Serial.print(c);

		// Prepair for next handling next character
		ctx->index++;
	}
}

// Adds value to EEPROM until max length is reached
void confUpdateValue(struct config_ctx *ctx, const char c) {
	// Stores the character in EEPROM if max value length is not reached
	if (ctx->index < ctx->len) {
		Serial.print(c);
		confWrite(ctx->addr + ctx->index, c);
	}
	// Max value length has been reached
	else if (ctx->index == ctx->len) {
		Serial.print("\nMaximum input length reached");
	}
	// Only handle max value length warning once
	else {
		return;
	}
	ctx->index++;
}

// Parses incomming data and updates matching conf items EEPROM value if key existed in "confList"
void confParse(struct config_ctx *ctx, bool *touched, const char c) {
	// Wraps up parsing of line
	if (c == '\n') {
		// Terminates string in EEPROM if max length was not reached
		if (ctx->addr != -1) {
			if (ctx->index < ctx->len) confWrite(ctx->addr + ctx->index, '\0');
		}
		// Message did not contain a delimiter character
		else if (ctx->index != 0) {
			Serial.print(" ] Aborted");
		}
		// Incoming message was empty
		else {
			// Commits changes to config data and reconnect everything if context has been touched
			if (*touched == 1) {
				*touched = 0;
				EEPROM.commit();
				Serial.print("Done. Restarting...\n\n");
				connect();
			}
			//!! prints
			else {
				Serial.print("Received no input data to store\n");
			}

			// No need to reset the context since it has not been altered
			return;
		}

		//!! prints
		Serial.print('\n');

		// Resets context
		*ctx = init_ctx;
	}
	// Validates key from message
	else if (ctx->addr == -1) {
		// Marks context as touched to precent untoched contexts from printing "Done"
		if (ctx->index == 0) {
			*touched = 1;
			Serial.print("[ ");
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
	Serial.print('\n');

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
			const unsigned long timeout = millis() + 500;
			while (!serialHasData()) {
				yield();
				if (timeout < millis()) {
					if (ctx.addr != -1 || ctx.index != 0) confParse(&ctx, &touched, '\n');
					confParse(&ctx, &touched, '\n');
					return;
				}
			}
		}
	}
}

// Gets string value from EEPROM address range
void confGet(struct config conf, char value[]) {
	for (int i = 0; i < conf.len; i++) {
		value[i] = confRead(conf.addr + i);
		if (value[i] == '\0') return;
	}
	value[conf.len] = '\0';
}



// Displays a progress bar and enables handling conf input over serial
void progressbar() {
	static unsigned long timeout = 0;
	yield();

	// Shows progress indicator every 200 milliseconds
	const unsigned long current = millis();
	if (timeout < current) {
		timeout = current + 200;
		Serial.print(".");
	}

	// Handles incoming serial data during progression
	if (serialHasData()) {
		Serial.print('\n');
		confSerialLoop();
		Serial.print("continuing...");
	}
}



// Size to use for indentation
const char tab[] = "    ";

// Wait for response data from host
bool socketWaitForResponse() {
	const unsigned long timeout = millis() + 10000;
	while (!socketHasData()) {
		if (timeout < millis()) return 0;
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



// Connects to the wifi network stored in the configuration
void connectNetwork() {
	// Gets ssid and ssidKey from EEPROM
	char ssid[conf_ssid.len + 1];
	confGet(conf_ssid, ssid);
	char ssidKey[conf_ssidKey.len + 1];
	confGet(conf_ssidKey, ssidKey);

	//!! prints
	Serial.print("Connecting to SSID: ");
	Serial.print(ssid);
	Serial.print("..");

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
	Serial.print('\n');

	//!! prints
	Serial.print("IP address: ");
	Serial.print(WiFi.localIP());
	Serial.print('\n');
}

// Connects to the host, establishes a WebSocket connection and sends a VerbalEyes authentication request
void connectSocket() {
	// Gets host from EEPROM
	char host[conf_host.len + 1];
	confGet(conf_host, host);

	//!! prints
	Serial.print("Connecting to Host: ");
	Serial.print(host);
	Serial.print(':');

	// Gets port from EEPROM
	unsigned short port = 0;
	for (short i = 0; i < conf_port.len; i++) {
		const char c = confRead(conf_port.addr + i);
		Serial.print(c);
		if (c == '\0') break;
		if (port > 6553 || port == 6553 && (c > '5' || c < '0')) {
			Serial.print("\nPort is outside range\n");
			return;
		}
		port = port * 10 + c - '0';
	}

	// Connects to host and retries if connection fails
	if (!socketConnect(host, port)) {
		Serial.print("\nConnection failed\n");
		return;
	}

	// Sets random seed
	//!! should be moved to its own function since its using micros()
	srand(micros());

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
	if (!socketWaitForResponse()) {
		Serial.print("\nTimed out waiting for HTTP response\n");
		socketClose();
		return;
	}
	Serial.print('\n');

	//!! prints
	Serial.print("Received HTTP response:\n");
	Serial.print(tab);

	// Matches the incoming HTTP response against required and illigal substrings
	bool EOL = 0;
	int matches[6] = {};
	while (socketHasData()) {
		yield();
		const char c = socketRead();
		if (c == '\n') EOL = 1;
		else if (c != '\r') {
			if (EOL == 1) {
				Serial.print('\n');
				Serial.print(tab);
				EOL = 0;
			}
			Serial.print(c);
		}
		socketMatchData(&matches[0], tolower(c), "http/1.1 101");
		socketMatchData(&matches[1], tolower(c), "connection: upgrade\r\n");
		socketMatchData(&matches[2], tolower(c), "upgrade: websocket\r\n");
		socketMatchData(&matches[3], c, accept);
		socketMatchData(&matches[4], c, "Sec-WebSocket-Extensions: ");
		socketMatchData(&matches[5], c, "Sec-WebSocket-Protocol: ");
	}
	Serial.print('\n');

	// Requires HTTP response code 101
	if (!matches[0]) {
		Serial.print("Received unexpected HTTP response code\n");
		socketClose();
		return;
	}

	// Requires "Connection" header with "Upgrade" value and "Upgrade" header with "websocket" value
	if (!matches[1] || !matches[2]) {
		Serial.print("HTTP response is not an upgrade to the WebSockets protocol\n");
		socketClose();
		return;
	}

	// Requires WebSocket accept header with correct value
	if (!matches[3]) {
		Serial.print("Missing or incorrect WebSocket accept header\n");
		socketClose();
		return;
	}

	// Checks for non-requested WebSocket extension header
	if (matches[4]) {
		Serial.print("Unexpected WebSocket Extension header\n");
		socketClose();
		return;
	}

	// Checks for non-requested WebSocket protocol header
	if (matches[5]) {
		Serial.print("Unexpected WebSocket Protocol header\n");
		socketClose();
		return;
	}

	//!! prints
	Serial.print("WebSocket connection established\n");

	// Gets project and project_key from EEPROM
	char proj[conf_proj.len + 1];
	confGet(conf_proj, proj);
	char projKey[conf_projKey.len + 1];
	confGet(conf_projKey, projKey);

	//!! prints
	Serial.print("Connecting to project: ");
	Serial.print(proj);

	// Sends VerbalEyes project authentication request
	char auth[8 + 33 + conf_proj.len + conf_projKey.len];
	sprintf(auth, "{\"_core\": {\"id\": \"%s\", \"pwd\": \"%s\"}}", proj, projKey);
	writeWebSocketFrame(auth);

	// Await authentication response from server
	if (!socketWaitForResponse()) {
		Serial.print("\nTimed out waiting for authentication response\n");
		socketClose();
		return;
	}
	Serial.print('\n');

	// Checks that this is an unfragmented WebSocket frame in text format
	if (socketRead() != 0x81) {
		Serial.print("Received response data is either not a WebSocket frame or uses an unsupported WebSocket feature\n");
		socketClose();
		return;
	}

	// This byte contains the maskBit and the payload length
	const char byte2 = socketRead();

	// Server is not allowed to mask messages sent to the client according to the spec
	if (byte2 >= 0x80) {
		Serial.print("Reveiced a masked frame which is not allowed\n");
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
		Serial.print("The received response data payload was too long\n");
		socketClose();
		return;
	}

	//!! prints
	Serial.print("Received authentication response:\n");
	Serial.print(tab);

	// Matches the incoming WebSocket string against required substring
	int authed = 0;
	for (int i = 0; i < payloadLen; i++) {
		yield();
		const char c = socketRead();
		Serial.print(c);
		socketMatchData(&authed, c, "\"authed\":");
	}
	Serial.print('\n');

	// Validates the authentication
	if (!authed) {
		Serial.print("Authentication Failed\n");
		socketClose();
		return;
	}

	//!! prints
	Serial.print("Authenticated\n\n");
}

// Connects to network and socket
void connect() {
	// Connects to the network and socket server using conf item values
	connectNetwork();
	connectSocket();

	// Retries to connect to socket server until succcess if it failed
	while (!socketStatus()) {
		Serial.print("Retrying to connect to the host in 3 seconds\n");
		const unsigned long timeout = millis() + 3000;
		while (timeout > millis()) {
			yield();
			if (serialHasData()) confSerialLoop();
		}
		connectSocket();
	}

}
