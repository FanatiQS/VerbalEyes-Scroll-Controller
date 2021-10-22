#include <stdbool.h> // bool
#include <stdint.h> // int8_t, uint8_t, int16_t, uint16_t
#include <stdlib.h> // size_t

// Include Guard
#ifndef VERBALEYES_SPEED_CONTROLLER_H
#define VERBALEYES_SPEED_CONTROLLER_H

// The version of the VerbalEyes SpeedController Micro Library
#define VERBALEYES_VERSION 0.5f

// Number of characters required for configuration
#define VERBALEYES_CONFIGLEN 269

// Makes functions work in C++
#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for functions used to interact with the system
int8_t verbaleyes_initialize();
bool verbaleyes_configure(const int16_t);
void verbaleyes_setspeed(const uint16_t);
void verbaleyes_resetoffset(const bool);

// Access to persistent storage
extern char verbaleyes_conf_read(const uint16_t);
extern void verbaleyes_conf_write(const uint16_t, const char);
extern void verbaleyes_conf_commit();

// Interacts with network interface
extern void verbaleyes_network_connect(const char*, const char*);
extern int8_t verbaleyes_network_connected();

// Interacts with the socket
extern void verbaleyes_socket_connect(const char*, const uint16_t);
extern int8_t verbaleyes_socket_connected();
extern int16_t verbaleyes_socket_read();
extern void verbaleyes_socket_write(const uint8_t*, const size_t);

// Logs data to an interface
extern void verbaleyes_log(const char*, const size_t);

// Ends extern c block
#ifdef __cplusplus
}
#endif

#endif // Ends include Guard
