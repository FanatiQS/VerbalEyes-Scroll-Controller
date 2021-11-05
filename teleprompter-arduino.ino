/*
An example of how to wire up a wemos d1 mini with a potentiometer to control speed and a button to jump to top.

COMPONENTS:
            Wemos D1 Mini
        TX               RST
        RX                A0
        D1                D0
        D2                D5
        D3                D6
        D4                D7
        GND               D8
        5V              3.3V


            Potentiometer
        VCC     Wiper    GND

            Button
        GND              VCC


CONNECTIONS:
        Wemos<GND> -- Potentiometer<GND> -- Button<GND>
        Wemos<3.3V> < -- > Potentiometer<VCC>
        Wemos<A0> -- Potentiometer<Wiper>
        Wemos<D3> -- Button<VCC>
*/

#include "./src/scroll_controller.h"



// Binds the VerbalEyes percistent storage calls to EEPROM library
#include <EEPROM.h>

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



// Binds the VerbalEyes network and socket calls to the ESP8266 WiFi library
#include <ESP8266WiFi.h>
// #include <WiFiClientSecure.h>

// Connects to a WiFi network
void verbaleyes_network_connect(const char* ssid, const char* key) {
	WiFi.begin(ssid, key);
}

// Gets the connection status of the WiFi connection
int8_t verbaleyes_network_connected() {
	switch (WiFi.status()) {
		default: return 0;
		case WL_CONNECTED: return 1;
		case WL_NO_SSID_AVAIL: {
			Serial.print("\r\nNo network found with that name");
			return -1;
		}
		case WL_CONNECT_FAILED: {
			Serial.print("\r\nThe password was incorrect");
			return -1;
		}
	}
}

// Creates a network socket to use for WebSocket communication with the server
WiFiClient client;
// WiFiClientSecure client; // Switch to this line when using HTTPS

// Connects the socket to an endpoint
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
	// client.setInsecure();  // Enable this line when using HTTPS
	client.connect(host, port);
}

// Gets the connection status of the socket connection
int8_t verbaleyes_socket_connected() {
	return (client.connected()) ? 1 : -1;
}

// Consumes a single character from the sockets response data buffer
short verbaleyes_socket_read() {
	return client.read();
}

// Sends a string to the endpoint the socket is connected to
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {
	client.write(str, len);
}



// Prints the logs to the serial interface
void verbaleyes_log(const char* str, const size_t len) {
	Serial.print(str);
}



// Fills missing clock function. Used for getting random seed
unsigned long clock() {
	return micros();
}

void setup() {
	WiFi.setAutoConnect(0);
	Serial.begin(9600);
	EEPROM.begin(VERBALEYES_CONFIGLEN);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(D3, INPUT_PULLUP);
}

void loop() {
	// Blinks built-in LED every 256ms or once for 256ms every 8192ms
	static long blinkMask = 0x100;
	static bool blinkState = 0;
	bool blinkValue = (millis() & blinkMask) != 0;
	if (blinkValue != blinkState) {
		blinkState = blinkValue;
		digitalWrite(LED_BUILTIN, blinkValue);
	}

	// Updates config data from serial input. Restarts loop if handling serial data
	if (verbaleyes_configure(Serial.read())) {
		blinkMask = 0x100;
		return;
	}

	// Ensure network and socket are setup and connected. Restarts loop if setup is not done
	if (verbaleyes_initialize()) {
		blinkMask = 0x100;
		return;
	}

	// Update server speed based on potentiometer at A0
	verbaleyes_setspeed(analogRead(A0));

	// Jump to top of document if button at pin 0 is pulled high
	verbaleyes_resetoffset(digitalRead(D3));

	// Blinks light every 8192ms when idle
	if (blinkMask != 0x1F00) {
		blinkMask = 0x1F00;
		digitalWrite(LED_BUILTIN, true);
	}

	// Only reads pins 25 times per second
	delay(1000 / 25);
}
