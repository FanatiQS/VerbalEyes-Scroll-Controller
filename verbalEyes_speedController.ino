#include <ESP8266WiFi.h>
//!! #include <WiFiClientSecure.h>
#include <EEPROM.h>



// Pin and board specific definitions
#define POT_MAX 1024
#define SAMPLERATE 3
#define LED_PIN LED_BUILTIN
#define SPEEDPIN A0
#define JUMPTOTOPPIN 0



void initialize();

//!!
void setup() {
	EEPROM.begin(512);
	Serial.begin(9600);
	//pinMode(LED_BUILTIN, OUTPUT);
	//pinMode(JUMPTOTOPPIN, INPUT_PULLUP);

	Serial.print("\n\n\n");
	Serial.println(getArray());
	initialize();
}


//!!
void loop() {
	// if (!socketStatus()) {
	// 	if (!networkStatus()) connectNetwork();
	// 	connectSocket();
	// }
	// if (serialHasData()) confSerialLoop();
	// updateSpeed();
	// jumpToTop();
}
extern "C" {
// Gets elapsed time in milliseconds
unsigned long getTime() {
	return millis();
}

// Gets a random seed to use
unsigned long randomSeed() {
	return micros();
}



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
char logString[1024] = "";
char* logStart = logString;
void serialWriteString(const char str[]) {
	//Serial.print(str);
	Serial.println(str);
}

//!!
void serialWriteChar(const char c) {
	Serial.print(c);
}



// Reads a character from address in EEPROM
char confRead(const int addr) {
	return EEPROM.read(addr);
}

// Writes character to address in EEPROM
void confWrite(const int addr, const char c) {
	EEPROM.write(addr, c);
}

// Commits changes made to EEPROM
void confCommit() {
	EEPROM.commit();
}



// Sets state of info LED
void infoLED(const bool state) {
	digitalWrite(LED_PIN, state);
}

// Reads analog signal for speed
int readSpeed() {
	return analogRead(SPEEDPIN);
}

// Reads digital signal for a pin
int readButton(const int pin) {
	return digitalRead(pin);
}
}
