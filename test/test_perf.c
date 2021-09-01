#include <stdio.h>
#include <sys/time.h>

#include "../src/scroll_controller.h"

#include "./helpers/print_colors.h"

// Use same random numbers for every test
int rand(void) { return 0; } //!! could replace clock instead since that is what sets the random seed

// Noop
void verbaleyes_network_connect(const char* ssid, const char* key) {}
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
void verbaleyes_conf_write(const unsigned short addr, const char c) {}
void verbaleyes_conf_commit() {}

// Is connected right away
int8_t verbaleyes_network_connected() { return 1; }
int8_t verbaleyes_socket_connected() { return 1; }

// Gets fake data
char verbaleyes_conf_read(const unsigned short addr) { return 0; }

// Response data
char socket_read_data[] = "\0http/1.1 101 OK\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ICX+Yqv66kxgM0FcWaLWlFLwTAI=\r\n\r\n\0\x81\x09" "authed789\0";
int socket_read_index = 0;
short verbaleyes_socket_read() {
	unsigned char c = socket_read_data[socket_read_index++];
	return (c == '\0') ? EOF : c;
}

void verbaleyes_log(const char* str, const size_t len) {
	//printf("%s", str);
}

int main(void) {
	struct timeval t1, t2;
	gettimeofday(&t1, NULL);
	verbaleyes_initialize();
	gettimeofday(&t2, NULL);
	float time = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec;
	printf("" COLOR_BLUE "Tested maximum performance:\n" COLOR_GREEN "%f seconds\n%f milliseconds\n%f microseconds\n" COLOR_NORMAL, time / 1000000, time / 1000, time);
	return 0;
}
