
## TODO:
* System to configure device from something other than usb? ESP can have both access point and connect to existing network. It could setup an access point and serve a configuration webpage that uses simple POST request with same structure as serial data. The issue is security. USB configuration is good since it requires physical access. One idea to help a little bit with security is to only enable the access point network for the first 10 seconds, if no one has connected to it, disable it again. There is still an issue with security for the fist 10 seconds though. There could be a password required, but that requires setting a password for the device and might be overlooked by most. It could also not set up the access point if the password is not set up. Initially, the network config does not work, but if the user wants it, they could enable it by configuring a password over usb.

* LED to indicate what is going on using the built in led during initilization and during setting conf data

* Is it required to "subscribe" to a document after we have been authenticated? We want to subscribe to default document anyway. This would only be needed if the server doesn't have the default document open and can not understand it should open it. Sample code: `{\"_core\": {\"sub\": \"\"}}`. Right now it is not used, but it was used in old code and is here just in case I would realize it is actually needed.

* Parsing:
	* Support for escape character when writing configuration data over serial? Sample from old version:
			static int esc = 0;
			// Escapes this character
			if (esc == 1) {
				esc = 0;
			}
			// Escapes next character
			else if (c == '\\') {
				esc = 1;
				return 0;
			}

	* Support comments, ignore that entire line. From old readme: "Comments are supported with # sign. Everything between # and EOL is ignored"

	* Support spaces in start and end. Trim them out from both key validation, value writing to eeprom and writing to serial? From old readme, this is a guideline for how it could (and has previously been) implemented:
		* Space trimming:
		*		All spaces before the key will be trimmed, therefore a key can not start with a space
		*		All spaces after the key will be trimmed unless there is a matching key, therefor a key can end with one or multiple spaces
		*		A key can contain spaces anywhere except at the start, but the end is safe
		*		The value only ever gets one space trimmed at the start
	* Old code:

			// Validates space character input for candidates
			if (c == ' ') {
				for (int i = KEYSLAST; i >= 0; i--) {
					if (valid[i] != 1) continue;
					if (keys[i][index] == '\0') valid[i] = -1;
					else if (keys[i][index] != c) valid[i] = 0;
				}
				spaces++;
			 }
			 // Validates other character matches for candidates
			 else {
				//!! print
				while (spaces > 1) {
					Serial.print(' ');
					spaces--;
				}
			}

* Add support for partial data on socket. When reading socket data, currently the processing stops after the sockets data buffer is empty. Should add some kind of check that if it would fail the checks later, it will wait 500ms or so to then give it another shot.

 * Define bool type if it is not defined already?

 * Create a nice update system. Previous idea that was not good because of control of what is updated: Add look for updates on host. Admins can upload the .bin file to the server and the arduino code will check if it needs updating. But no central server updating to prevent introducing bugs without actively making the choice to update

## NOTES FROM OLD FILE

   * Errors:
   *		Every key has a maximum length but everything up until the max length is reached will still be written to the EEPROM. This can result in incomplete and there for invalid data stored
   *		If a key is not found, the input will be ignored until it gets an EOL character. This is by design

   * EOL finnishes parsing and writing conf data. If data is incomplete, it discards current data and makes it possible to submit a new input. It can be inserted anywhere before delimiter to restart
   * Valid keys are anything that does not contain an "="
   * The syntax is similar to INI or ENV, so values are not put in quotes. Spaces in keys do not need to be escaped. In fact, escaping is not supported as of now.
   * INI sections are not supported
   * Keys are case sensitive
   * Syntax (not valid: comments and speces are not supported and ; is not EOL): # comments are ignored;[spaces >= 0][key][spaces >= 0]=[spaces 0-1][value]

## LED INDICATION Notes:
   * writing to conf

   * network connecting

   * port outside range			- bad host/port		-	configuration issue and should not occur with configuration software
   * host connection failed		- bad host/port		-	no server at host:port
   * http response timeout		- bad host/port		-	no responce from server
   * websocket response bad		- bad host/port		-	server does not know websocket
   * authentication timeout		- bad host/port		-	server did not send a websocket response
   * websocket format bad		- bad host/port		-	server fucked up websocket frame
   * websocket payload too long	- bad host/port		-	server sent a really long message

   * authentication failed		- bad proj/proj_key	-	no such project or wrong password

   * standby

   * bad host/port: There might not be a VerbalEyes server at host:port or the server is not acting propperly.

## Some Arduino/ESP functions used in the code that would need to be replaced if using something else
 * millis() // for timeouts
 * micros() // for setting random seed
 * yield() // for doing network stuff during loops

 * Libraries and types already defined in Arduino
 	* #include <stdlib.h> // rand, srand
 	* #include <string.h> // strlen
 	* #include <stdio.h> // sprintf
 	* #include <ctype.h> // tolower
 	* #include <stdint.h> // uint8_t
 	* bool type like: typedef unsigned char bool;
