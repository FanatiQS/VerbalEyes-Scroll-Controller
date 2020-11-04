#include <ESP8266WiFi.h>;
#include <EEPROM.h>
#include <DNSServer.h>
#include "./verbalEyes_speed_controller.h"



// Reads a character from the specified address in EEPROM
char verbaleyes_conf_read(const unsigned short addr) {
  return EEPROM.read(addr);
}

// Writes a character to the specified address in EEPROM
void verbaleyes_conf_write(const unsigned short addr, const char c) {
  EEPROM.write(addr, c);
}

// Commits changes made in EEPROM to flash
void verbaleyes_conf_commit() {
  EEPROM.commit();
}



// Connects to a WiFi network
void verbaleyes_network_connect(const char* ssid, const char* key) {
  WiFi.begin(ssid, key);
}

// Gets the connection status of the WiFi connection
bool verbaleyes_network_connected() {
  return WiFi.isConnected();
}

// Gets the local ip address for printing
uint32_t verbaleyes_network_getip() {
  return WiFi.localIP();
}


// Creates a network socket to use for WebSocket communication with the server
WiFiClient client;

// Connects the socket to an endpoint
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
  client.connect(host, port);
}

// Gets the connection status of the socket connection
bool verbaleyes_socket_connected() {
  return client.connected();
}

// Consumes a single character from the sockets response data buffer
short verbaleyes_socket_read() {
  return client.read();
}

// Sends a string to the endpoint the socket is connected to
void verbaleyes_socket_write(const char* str, const unsigned int len) {
  client.write(str, len);
}



// Prints a string to the serial interface
void verbaleyes_log(const char* str, const uint32_t len) {
  Serial.print(str);
}



// Fills missing clock function. Used for getting random value
unsigned long clock() {
  return micros();
}

void setup() {
	Serial.begin(9600);
	EEPROM.begin(CONFIGLEN);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(0, INPUT_PULLUP);
}

void loop() {
	while (updateConfig(Serial.read())); // Updates config data from serial input
	if (ensureConnection()) return; // Ensure network and socket are setup and connected. Restart loop if setup is not done
	updateSpeed(analogRead(A0)); // Update server speed based on potentiometer at A0
	jumpToTop(digitalRead(0)); // Jump to top of document if button at pin 0 is pulled high
	delay(1000 / 25); // Only read pins 25 times per second
}
