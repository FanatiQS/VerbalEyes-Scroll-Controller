#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <termios.h>

#include "../../verbalEyes_speed_controller.h"

/*
 * Hotkeys:
 *	Arrow_Up:		Speed up scroll speed
 *	Arrow_Down:		Slow down scroll speed
 *	Arrow_Right:	Network connected
 *	Arrow_Left:		Socket connected (if actual socket is connected)
 */



#define POTMAX 32

bool wifiConnected = 0;
bool socketConnected = 0;
uint16_t potSpeed = 0;

// Reads one character from the standard in if it has anything
int16_t readFromSocket(int fd) {
	unsigned char c = 0;
	read(fd, &c, 1);

	// Returns character unless it is escape character
	if (c != '\e') return c;

	// Gets escaped sequence
	read(fd, &c, 1);
	if (c != '[') return EOF;
	read(fd, &c, 1);
	switch (c) {
		// Up
		case 'A': {
			if (potSpeed == POTMAX) break;
			potSpeed++;
			break;
		}
		// Down
		case 'B': {
			if (potSpeed == 0) break;
			potSpeed--;
			break;
		}
		// Right
		case 'C': {
			wifiConnected = !wifiConnected;
			break;
		}
		// Left
		case 'D': {
			if (!wifiConnected) break;
			socketConnected = !socketConnected;
			break;
		}
	}

	return EOF;
}



// The virual EEPROM
char EEPROM[CONFIGLEN];

// Reads a character from the specified address in fake EEPROM
unsigned char verbaleyes_conf_read(const unsigned short addr) {
	return EEPROM[addr];
}

// Writes a character to the specified address in fake EEPROM
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	EEPROM[addr] = c;
}

// Commits changes made in EEPROM, nothing to be done
void verbaleyes_conf_commit() {}



// Connects to a WiFi network, nothing to be done
void verbaleyes_network_connect(const char* ssid, const char* key) {}

// Gets the fake connection status of the WiFi connection
bool verbaleyes_network_connected() {
	return wifiConnected;
}

// Gets random 32bit value to represent fake ip address
uint32_t verbaleyes_network_getip() {
	srand(clock() * clock());
	return rand();
}



// The tcp socket and its connection status
int sockfd = 0;
bool sockstatus = 0;

// Connects the socket to the endpoint
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
	if (sockfd != 0) close(sockfd);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Unable to create socket\n");
		exit(0);
	}

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(host);
	servaddr.sin_port = htons(port);

	sockstatus = !connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
}

// Gets the fake and real connection status of the socket connection
bool verbaleyes_socket_connected() {
	return sockstatus && socketConnected;
}

// Consumes a single character from the sockets response data buffer
short verbaleyes_socket_read() {
	return readFromSocket(sockfd);
}

// Sends a packet to the endpoint the socket is connected to
void verbaleyes_socket_write(const uint8_t* packet, const size_t len) {
	write(sockfd, packet, len);
}



// Prints the logs to standard out
void verbaleyes_log(const char* str, const size_t len) {
	printf("%s", str);
	fflush(stdout);
}



// Some kind of raw mode reset
struct termios orig_termios;
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Sends a string to config instead of char by char
void updateConfig_str(char* str) {
	while (*str != '\0') {
		updateConfig(*str);
		str++;
	}
}

//!!
int main() {
	// Sets STDIN to be unbuffered
	tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	// Configure initial configuration
	updateConfig_str("ssid=myWiFi\n");
	updateConfig_str("host=127.0.0.1\n");
	updateConfig_str("port=8080\n");
	updateConfig_str("path=/\n");
	updateConfig_str("proj=myProject\n");
	updateConfig_str("speedmin=-10\n");
	updateConfig_str("speedmax=10\n");
	EEPROM[266] = POTMAX;
	updateConfig('\n');

	// Main loop
	while (1) {
		if (updateConfig(readFromSocket(0))) continue;
		if (ensureConnection()) continue;
		updateSpeed(potSpeed);
		// jumpToTop(digitalRead(0));
	}
	return 0;
}
