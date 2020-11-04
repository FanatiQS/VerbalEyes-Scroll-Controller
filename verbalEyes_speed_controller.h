// Include Guard
#ifndef VERBALEYES_SPEED_CONTROLLER_H
#define VERBALEYES_SPEED_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

// Number of characters required for configuration
#define CONFIGLEN 238 // Needs to be updated manually

// Returing states of ensureConnection
#define CONNECTIONFAILED -1
#define CONNECTING 1
#define CONNECTED

// Makes functions work in c++
#ifdef __cplusplus
extern "C" {
#endif

//!!
int8_t ensureConnection();
bool updateConfig(const int16_t);
void updateSpeed(const uint16_t);
void jumpToTop(const bool);

// Access to persistent storage
extern char confRead(const uint16_t);
extern void confWrite(const uint16_t, const char);
extern void confCommit();

//!!
extern void networkConnect(const char*, const char*);
extern bool networkConnected();
extern uint32_t networkGetIP();

//!!
extern void socketConnect(const char*, const uint16_t);
extern bool socketConnected();
extern int16_t socketRead();
extern void socketWrite(const char*, const uint32_t);

//!!
extern void serialPrint(const char*, const uint32_t);

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
 * ignorejitter: The sensitivity of the readings in percent of entire range. When analog reads are not stabile, this can be raised to not read small changes.
 */
