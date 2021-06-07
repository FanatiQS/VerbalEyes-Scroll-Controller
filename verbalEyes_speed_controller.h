#include <stdbool.h>
#include <stdint.h>

// Include Guard
#ifndef VERBALEYES_SPEED_CONTROLLER_H
#define VERBALEYES_SPEED_CONTROLLER_H

// Number of characters required for configuration
#define CONFIGLEN 267 // Needs to be updated manually

// Returing states of ensureConnection
#define CONNECTIONFAILED -1
#define CONNECTING 1
#define CONNECTED 0

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
extern unsigned char verbaleyes_conf_read(const uint16_t);
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


/*
 * Configuration items available
 *
 * ssid:
 *		Description:
 *			The SSID (WiFi) to connect to
 *		Type:
 *			ASCII
 *		Max Length:
 *			32 characters
 * ssidkey:
 *		Description:
 *			The password for the SSID
 *		Type:
 * 			ASCII
 *		Max Length:
 * 			64 characters
 * host:
 *		Description:
 * 			The IP address or URL to the verbalEyes server the device should connect to
 *		Type:
 * 			ASCII
 *		Max Length:
 *			32 characters
 * port:
 * 		Description:
 *			The port to use when connecting to the host (usually 80 for http and 443 for https)
 *		Type:
 *			Unsigned short
 *		Maximuim Value:
 *			65535
 *		Minimum Value:
 *			0
 * path:
 *		Description:
 *			The path used to connect to the verbalEyes server, usually just "/" for root
 *		Type:
 *			ASCII
 *		Max Length:
 *			32 characters
 * proj:
 *		Description:
 *			The verbalEyes project to connect to
 *		Type:
 *			ASCII
 *		Max Length:
 *			32 characters
 * projkey:
 *		Description:
 *			The password for the project
 *		Type:
 *			ASCII
 *		Max Length:
 *			32 characters
 * speedmin:
 *		Description:
 *			The scroll speed to use when the dial is turned all the way in one direction
 *		Type:
 *			signed short
 *		Maximum value:
 *			32767
 *		Minimum value:
 *			-32767
 * speedmax:
 *		Description:
 *			The scroll speed to use when the dial is turned all the way in the other direction
 *		Type:
 *			signed int
 *		Maximum value:
 *			32767
 *		Minimum value:
 *			-32767
 * deadzone:
 *		Description:
 *			The size of the zero mark in percent of entire range
 *		Type:
 *			percentage
 *		Maximum Value:
 *			100
 *		Minimum Value:
 *			0
 *		Note:
 *			Percent sign (%) should not be included
 * callow:
 *		Description:
 *			The smalles value received from analog read. Useful when analog values don't go all the way down to 0
 *		Type:
 *			unsigned short
 *		Maximum Value:
 *			65535
 *		Minimum Value:
 *			0
 * calhigh:
 *		Description:
 *			The largest value received from analog read. For 10 bit readings, this should be 1024 but might not reach all the way
 *		Type:
 *			unsigned short
 *		Maximum Value:
 *			65535
 *		Minimum Value:
 *			0
 * sensitivity:
 *		Description:
 *			The sensitivity of the readings in percent of entire range. When analog reads are not stabile, this can be raised to not read small jitter fluctuations
 *		Type:
 *			percentage
 *		Maximum Value:
 *			100
 *		Minimum Value:
 *			0
 *		Note:
 *			Percent sign (%) should not be included
 */
