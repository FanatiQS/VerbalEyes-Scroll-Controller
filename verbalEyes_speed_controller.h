#include <stdint.h>
#include <stdbool.h>

// Include Guard
#ifndef VERBALEYES_SPEED_CONTROLLER_H
#define VERBALEYES_SPEED_CONTROLLER_H

// Number of characters required for configuration
#define CONFIGLEN 238 // Needs to be updated manually

// Number of seconds before unfinished configuration input times out
#ifndef CONFIGTIMEOUT
#define CONFIGTIMEOUT 5
#endif

// Returing states of ensureConnection
#define CONNECTIONFAILED -1
#define CONNECTING 1
#define CONNECTED 0

// Number of seconds before unsuccessful connection times out
#ifndef CONNECTINGTIMEOUT
#define CONNECTINGTIMEOUT 10
#endif

// Number of seconds to delay retrying after connection has failed
#ifndef CONNECTIONFAILEDDELAY
#define CONNECTIONFAILEDDELAY 5
#endif

// Makes functions work in C++
#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for functions used to interact with the system
int8_t ensureConnection();
bool updateConfig(const int16_t);
void updateSpeed(const uint16_t);
void jumpToTop(const bool);

// Access to persistent storage
extern char verbaleyes_conf_read(const uint16_t);
extern void verbaleyes_conf_write(const uint16_t, const char);
extern void verbaleyes_conf_commit();

// Interacts with network interface
extern void verbaleyes_network_connect(const char*, const char*);
extern bool verbaleyes_network_connected();
extern uint32_t verbaleyes_network_getip();

// Interacts with the socket
extern void verbaleyes_socket_connect(const char*, const uint16_t);
extern bool verbaleyes_socket_connected();
extern int16_t verbaleyes_socket_read();
extern void verbaleyes_socket_write(const uint8_t*, const size_t);

// Logs data to an interface
extern void verbaleyes_log(const char*, const size_t);

// Ends extern c block
#ifdef __cplusplus
}
#endif

#endif // Ends include Guard


/* Configuration items available
 *
 * ssid: The SSID (WiFi) to connect to.
 * ssidkey: The password for the SSID.
 * host: The IP address or URL to the verbalEyes server the device should connect to.
 * port: The port to use when connecting to the host (usually 80 for http and 443 for https).
 * path: The path used to connect to the verbalEyes server, usually just "/".
 * proj: The verbalEyes project to connect to.
 * projkey: The password for the project.
 * speedmin: The scroll speed to use when the dial is turned all the way in one direction.
 * speedmax: The scroll speed to use when the dial is turned all the way in the other direction.
 * deadzone: The size of the zero mark in percent of entire range.
 * callow: The smalles value received from analog read. Useful when analog values don't go all the way down to 0.
 * calhigh: The largest value received from analog read. For 10 bit readings, this should be 1024 but might not reach all the way.
 * sensitivity: The sensitivity of the readings in percent of entire range. When analog reads are not stabile, this can be raised to not read small jitter fluctuations.
 */
