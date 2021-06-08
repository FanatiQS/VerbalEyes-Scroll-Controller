#include <stdio.h>
#include <stdlib.h>
#include "../verbalEyes_speed_controller.h"

// The array representing the eeprom
char conf_eeprom[CONFIGLEN];

// VerbalEyes function to write data to eeprom
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	conf_eeprom[addr] = c;
}

// Bool indicating if commit has happened or not
bool conf_commited = 0;

// VerbalEyes function to commit data to eeprom
void verbaleyes_conf_commit() {
	conf_commited = 1;
}
