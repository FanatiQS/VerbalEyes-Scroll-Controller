#ifndef EEPROM_H
#define EEPROM_H
#include <stdbool.h>
#define CONFFLAGCOMMITED 1
#define CONFFLAGOUTSIDESTR 2
void configure_str(const char* str);
void conf_cmp(const int, const char*, const int);
void conf_matchcommit(bool);
void conf_is_empty();
void conf_clear();
void conf_setflags(const int);
extern char confBuffer[];
#endif
