#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../verbalEyes_speed_controller.h"

#include "./eeprom.h"
#include "./log.h"



// Only defined to not throw compilation errors
void verbaleyes_network_connect(const char* ssid, const char* key) {}
bool verbaleyes_network_connected() { return 1; }
uint32_t verbaleyes_network_getip() { return 1; }
void verbaleyes_socket_connect(const char* host, const unsigned short port) {}
bool verbaleyes_socket_connected() { return 0; }
short verbaleyes_socket_read() { return 1; }
void verbaleyes_socket_write(const uint8_t* str, const size_t len) {}
unsigned char verbaleyes_conf_read(const unsigned short addr) { return 1; }



// Prints string to config update system character by character
void updateConfig_str(const char* str) {
	for (int i = 0; i < strlen(str); i++) {
		updateConfig(str[i]);
	}
}



// Tests if macro CONFIGLEN is correct length
void test_conf_configlen() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Updates config
	updateConfig_str("sensitivity=65535\n\n");

	// Checks if update to last config item wrote to last 2 eeprom bytes
	if (conf_eeprom[CONFIGLEN - 3] == 1 && conf_eeprom[CONFIGLEN - 2] == -1 && conf_eeprom[CONFIGLEN - 1] == -1) {
		printf("Test successful: configlen macro\n");
		return;
	}

	// Logs debug info if it failed test
	printf("\nFAILED: CONFIGLEN = %i failed its test\nAddress\tValue\n", CONFIGLEN);
	for (int i = CONFIGLEN - 10; i < CONFIGLEN; i++) {
		printf("%i\t%i\n", i, conf_eeprom[i]);
	}
}

// Tests if it is commiting when it should
void test_conf_commit() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Resets commit indicator
	conf_commited = 0;

	// Writes data to eeprom
	updateConfig_str("ssid=123\n");

	// Logs debug info if it failed test
	if (conf_commited != 0) {
		printf("\nFAILED: Did commit before it should have\n");
		return;
	}

	// Completes update
	updateConfig('\n');

	// Logs debug info if it failed test
	if (conf_commited != 1) {
		printf("\nFAILED: Did not commit after double LF\n");
		return;
	}

	printf("Test successful: config_commit\n");
}

// Tests if it is timing out in key
void test_conf_timeout() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Fails updating config with timeout in the middle
	updateConfig_str("ss");
	updateConfig(0);
	updateConfig(0);
	updateConfig_str("id=123\n\n");

	// Logs debug info if it failed test
	if (conf_eeprom[0] != 1) {
		printf("\nFAILED: Data was written to eeprom for timeout test\n");
		return;
	}

	printf("Test successful: config_timeout\n");
}

// Test if it is timing out in value
void test_conf_timeout2() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Fails updating config with timeout in the middle
	updateConfig_str("ssid=123");
	updateConfig(0);
	updateConfig(0);
	updateConfig_str("456\n\n");

	// Logs debug info if it failed test
	if (memcmp(conf_eeprom, "123\0", 4)) {
		printf("\nFAILED: Data was written to eeprom for timeout2 test\n");
		return;
	}

	printf("Test successful: config_timeout2\n");
}

// Test if it is completing right away with NULL or LF
void test_conf_empty() {
	if (updateConfig(0) == 0 && updateConfig('\n') == 0 && updateConfig('\n') == 0) {
		printf("Test successful: config_empty\n");
		return;
	}

	printf("\nFAILED: Writing NULL as first char did not complete update right away\n");
}

// Test if it is working correctly with delimiter as first char
void test_conf_nokey() {
	// Fails updating config with timeout in the middle
	updateConfig_str("=123\n\n");

	printf("Test unknown: config_nokey\n");
}

// Test if it is working correctly with invalid key
void test_conf_invalidkey() {
	// Fails updating config with timeout in the middle
	updateConfig_str("banana=123\n\n");

	printf("Test unknown: config_nokey\n");
}

// Test if it is updating eeprom value correctly
void test_conf_update() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Fails updating config with timeout in the middle
	updateConfig_str("ssid=123\n\n");

	//!!
	if (memcmp(conf_eeprom, "123\0", 4)) {
		printf("\nFAILED: value was not update correctly\n");
	}

	printf("Test successful: config_update\n");
}

// Test if it is updating eeprom correctly for multiple updates
void test_conf_updatemult() {
	// Clears eeprom before test
	memset(conf_eeprom, 1, CONFIGLEN);

	// Fails updating config with timeout in the middle
	updateConfig_str("ssid=123\nssidkey=456\n\n");

	// Check first update
	if (memcmp(conf_eeprom, "123\0", 4)) {
		printf("\nFAILED: Value was not update correctly for ssid\n");
	}

	// Check second update
	if (memcmp(conf_eeprom + 32, "456\0", 4)) {
		printf("\nFAILED: Value was not update correctly for ssidkey\n");
	}

	printf("Test successful: config_updatemult\n");
}

//!! valid key with no delimiter `ssid\n\n`

//!! invalid key with no delimiter `banana\n\n`

//!! valid key with delimiter and no value `ssid=\n\n`

//!! overflow string `ssid=1234567890123456789012345678901234567890`

//!! delimiter char in value `ssid====`



//!! update number under 256, and over 256 and negative

//!! update number above max and under min for both signed and unsigned

//!! update w/ not number char with data after `port=a123\n\n`



//!! test verbaleyes_conf_read by calling ensureConnection once to read ssid and ssidkey and check them in verbaleyes_socket_connect



//!!
int main(void) {
	log_print = 1;
	log_print = 0;
	test_conf_nokey();
	test_conf_invalidkey();
	test_conf_update();
	test_conf_updatemult();
	test_conf_empty();
	test_conf_timeout();
	test_conf_timeout2();
	test_conf_commit();
	test_conf_configlen();
	return 0;
}
