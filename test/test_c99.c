#include "../verbalEyes_speed_controller.h"

// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
bool verbaleyes_network_connected() { return 1; }
uint32_t verbaleyes_network_getip() { return 1; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
bool verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 1; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
unsigned char verbaleyes_conf_read(const unsigned short addr) { return 1; }
void verbaleyes_conf_write(const unsigned short addr, const char c) {}
void verbaleyes_conf_commit() {}
void verbaleyes_log(const char* str, const size_t len) {}

int main(void) {
	return 0;
}
