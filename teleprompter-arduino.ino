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

#include <ESP8266WiFi.h>
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
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {
	client.write(str, len);
}



//!!
#include <ESP8266WebServer.h>
ESP8266WebServer confServer(80);

// Prints a string to the serial interface
void verbaleyes_log(const char* str, const size_t len) {
	Serial.print(str);
	if (confServer.client()) confServer.sendContent(str, len);
}



// Fills missing clock function. Used for getting random seed
unsigned long clock() {
	return micros();
}


//!!
//DNSServer dnsServer;

const String htmlform = "<!DOCTYPE html>\
<html>\
	<head>\
		<style>\
			body {\
				text-align: center;\
			}\
			form {\
				display: inline-block;\
				max-width: 300px;\
				width: 100%;\
			}\
			form * {\
				display: block;\
			}\
			form input, form button {\
				margin-bottom: 2em;\
				width: 100%;\
				box-sizing: border-box;\
			}\
		</style>\
	</head>\
	<body>\
	<h1>Configure your scroll-thingimajigg</h1>\
	<form method=\"post\" enctype=\"text/plain\">\
		<label for=\"ssid\">Wi-fi:</label>\
		<input id=\"ssid\" name=\"ssid\" disabled>\
		<label for=\"ssidkey\">Wi-fi passowrd:</label>\
		<input id=\"ssidkey\" name=\"ssidkey\">\
		<label for=\"host\">Host:</label>\
		<input id=\"host\" name=\"host\">\
		<label for=\"port\">Port:</label>\
		<input id=\"port\" name=\"port\">\
		<label for=\"proj\">Project:</label>\
		<input id=\"proj\" name=\"proj\">\
		<label for=\"projkey\">Project passowrd:</label>\
		<input id=\"projkey\" name=\"projkey\">\
		<br>\
		<label for=\"speedmax\">Left scroll speed:</label>\
		<input id=\"speedmax\" name=\"speedmax\">\
		<label for=\"speedmin\">Right scroll speed:</label>\
		<input id=\"speedmin\" name=\"speedmin\">\
		<label for=\"deadzone\">Deadzone size:</label>\
		<input id=\"deadzone\" name=\"deadzone\">\
		<br>\
		<label for=\"callow\">Calibrate left:</label>\
		<input id=\"callow\" name=\"callow\">\
		<label for=\"calhigh\">Calibrate right:</label>\
		<input id=\"calhigh\" name=\"calhigh\">\
		<label for=\"sensitivity\">Calibrate sensitivity size:</label>\
		<input id=\"sensitivity\" name=\"sensitivity\">\
		<button>\"Submit\"</button>\
	</form>\
	</body>\
</html>";

//!!
void handleRoot() {
	switch (confServer.method()) {
		case HTTP_GET: {
			confServer.send(200, "text/html", htmlform);
			return;
		}
		case HTTP_POST: {
			const String str = confServer.arg("plain");
			const uint32_t len = str.length();
			for (uint32_t i = 0; i < len; i++) {
				updateConfig(str.charAt(i));
			}
			updateConfig('\n');
			updateConfig('\n');
			confServer.sendContent("");
			confServer.client().stop();
			return;
		}
		default: {
			confServer.send(400);
		}
	}
}

//!!
void setup() {
	Serial.begin(9600);
	EEPROM.begin(CONFIGLEN);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(0, INPUT_PULLUP);
	// WiFi.mode(WIFI_STA);
	//WiFi.softAP("VerbalEyes_Scroller_Config7");
	//Serial.println(WiFi.softAPIP());
	//dnsServer.start(53, "*", WiFi.softAPIP());

	confServer.on("/", handleRoot);
	confServer.begin();

	// initialize wifi soft access point
	// initialize captive portal
}

void loop() {
	//confServer.handleClient();
	while (updateConfig(Serial.read())) yield(); // Updates config data from serial input
	if (ensureConnection()) return; // Ensure network and socket are setup and connected. Restart loop if setup is not done
	updateSpeed(analogRead(A0)); // Update server speed based on potentiometer at A0
	jumpToTop(digitalRead(0)); // Jump to top of document if button at pin 0 is pulled high
	delay(1000 / 25); // Only read pins 25 times per second
}


/*
   An interestin way to neither use cookies, paths, authorization header or hidden body fields is to track the ip address.
   When a user authenticates, their ip address gets stored in a static variable.
   They can later send a post request to update configuration data.
   When a post request is received, the static variable gets cleared and for anyone to update anything, they need to authenticate again.
   A path can also be used to clear ip address and have a link on the page that would send a get requeset to that path and clear the ip aka logout
   Might need some check to see that the device and the client are on the same network.
*/


