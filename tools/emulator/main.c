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



// Reads one character from the standard in if it has anything
int16_t readFromSocket(int fd) {
	unsigned char c;
	read(fd, &c, 1);
	return c;
}



// The virual EEPROM
char EEPROM[CONFIGLEN];

// Reads a character from the specified address in EEPROM
unsigned char verbaleyes_conf_read(const unsigned short addr) {
	return EEPROM[addr];
}

// Writes a character to the specified address in EEPROM
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	EEPROM[addr] = c;
}

// Commits changes made in EEPROM to flash
void verbaleyes_conf_commit() {}



// Number of seconds to delay before being considered connected to wifi
size_t wifitimeout;

// Connects to a WiFi network
void verbaleyes_network_connect(const char* ssid, const char* key) {
	if (strcmp(ssid, "myWifi")) return;
	if (strcmp(key, "password123")) return;
	wifitimeout = time(NULL) + 2;
}

// Gets the connection status of the WiFi connection
bool verbaleyes_network_connected() {
	if (!wifitimeout) return 0;
	return wifitimeout < time(NULL);
}

//!! Gets the local ip address for printing
// Gets random 32bit value to represent fake ip address
uint32_t verbaleyes_network_getip() {
	srand(clock() * clock());
	return rand();
}



// The tcp socket and its connection status
int sockfd = 0;
bool sockstatus = 0;

// Connects the socket to an endpoint
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

// Gets the connection status of the socket connection
bool verbaleyes_socket_connected() {
	return sockstatus;
}

// Consumes a single character from the sockets response data buffer
short verbaleyes_socket_read() {
	return readFromSocket(sockfd);
}

// Sends a string to the endpoint the socket is connected to
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {
	write(sockfd, str, len);
}



// Prints the logs to standard out
void verbaleyes_log(const char* str, const size_t len) {
	printf("%s", str);
	fflush(stdout);
}



//!!
struct termios orig_termios;
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
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

	strcpy(EEPROM, "myWifi");
	strcpy(EEPROM + 32, "password123");
	strcpy(EEPROM + 95, "127.0.0.1");
	EEPROM[159] = 31;
	EEPROM[160] = (signed char)144;
	strcpy(EEPROM + 161, "/");

	while (1) {
		time(NULL); // Why doesn't it work without this line?!?
		if (updateConfig(readFromSocket(0))) continue;
		if (ensureConnection()) continue;
		usleep(10000000 / 25);
		// updateSpeed(analogRead(A0));
		// jumpToTop(digitalRead(0));
	}
	return 0;
}
