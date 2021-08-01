# VerbalEyes SpeedController Core
This is a C library to control scroll speed and scroll position for a VerbalEyes server with a micro controller.
It works by connecting to Wi-Fi and sending the commands to the server over a WebSocket connection.
Configuration of what network and server to use is done with a simple text based protocol.

# Pre-built for Arduino/ESP
This is an implementation or the library that works on Arduino supported boards.

## Wiring
TODO: Add details on how the board should be wired

# Core Library
The core library should be able to run on anything supporting C99 (note that VLA support is required).
Because of this, the functions to access to Wi-Fi, persistent storage and logging are not defined.
These have to be implemented to work on the micro controller that it is used on by defining all the required functions the library uses. These functions are prototyped in `verbalEyes-speed-controler.h`.

## Defining required functions
### Persistent Storage
#### char verbaleyes_conf_read(const uint16_t addr)
Required by: ensureConnection
This function reads configuration data, character by character. The function is called with a specified address and the function should return the character at that address in the persistent storage.

#### void verbaleyes_conf_write(const uint16_t addr, const char value)
Required by: updateConfig
This function writes new configuration data to the persistent storage. The function is called with a specified address and a one byte integer. The function should write that integer to the specified address in the persistent storage.

#### void verbaleyes_conf_commit()
Required by: updateConfig
This function commits the updated configuration to persistent storage if micro controller requires it. Writing to EEPROM does not require this function to do anything, but writing to flash storage should be done in one go instead of writing each character as they come in.

### Network

TODO: Add documentation for all network functions

### Logging

#### void verbaleyes_log(const char* str, const size_t len)
Required by: ensureConnection
This function gets a string that should be passed to the logging function. A length argument is also defined but not not always required.

## Update Configuration
TODO: Add details on how to implement updateConfig
More details on how the input data should be structured can be found in the [configuration](#configuration) chapter.

## Connection
TODO: Add details about how to implement ensureConnection function

## Scroll Control
TODO: Add details about how to implement updateSpeed and jumpToTop functions


# Configuration
* Configuration of the device is done with a simple text based serial protocol consisting of a key-value-pair structure like this `key=value\n` where `\n` has to be encoded as an actual LF character.
* Spaces before or after the key or value are not trimmed out, so `key = value\n` would not be the same as `key=value\n`.
* Multiple configurations can be chained like this `key1=value1\nkey2=value2\n`.
* An empty line indicates configuration is done, so sending an extra `\n` at the end exits configuration mode.
* If a LF is not sent to exit configuration mode, it will automatically exit after 5 seconds.
* An alternative for the `=` delimiter is to use a tab instead.
* If input value is longer than the max length for that key, the configuration system will not add the overflowing data but everything up until max length is reached will be written to persistent storage. This will result in incorrect data in storage if error occurred.
* Comments are supported if the first character is a `#` sign, everything up to next LF will be ignored.
* Keys are case sensitive.

## Configuration item types
These are the types for configuration items
| Type 				| Minimum Value | Maximum Value | Description |
| ----------------- | ------------- | ------------- | -
| string 			| n/a 			| n/a 			| A string is a just a plain string
| unsigned short 	| 0 			| 65535 		| An unsigned short is a string representation of a 16 bit unsigned integer.
| signed short 		| -32767 		| 32767 		| A signed short is a string representation of a 16 bit signed integer. A `-` character is used before the number to indicate it is negative.
| percent 			| 0 			| 100 			| A percent value is a string representation of a percentage value not including the `%` sign. Values above 100 are technically possible but but have undefined behaviour.

## Configuration items
These are all the configurations that can be configured
| Key			| Type 				| Max length 	| Description |
| ------------- | ----------------- | ------------- | -
| ssid 			| string			| 32			| The SSID (WiFi) to connect to.
| ssidkey 		| string 			| 64			| The passphrase to the SSID.
| host 			| string 			| 64			| The host (server) to connect to. Can be an IP address or a DNS name.
| port 			| unsigned short 	| n/a			| The port to use when connecting to the host.
| path 			| string 			| 32			| The path to connect to on the host.
| proj 			| string 			| 32 			| The VerbalEyes project to connect to.
| projkey 		| string 			| 32 			| The key to the VerbalEyes project.
| speedmin 		| signed short 		| n/a 			| The speed to send when potentiometer is all the way in one direction.
| speedmax 		| signed short 		| n/a 			| The speed to send when potentiometer is all the way in the other direction.
| deadzone 		| percent 			| n/a 			| The size of the deadzone around the speed value 0. Used to make 0 mark bigger on the potentiometer.
| callow 		| unsigned short 	| n/a 			| The minimum value from the analog read. Used for calibrating potentimeter when it does not give 0 at the limit.
| calhigh 		| unsigned short 	| n/a 			| The maximum value from the analog read. Used for calibrating the maximum value from the potentiometer. Depends on quality of potentiometer and resolution of ADC on micro controller.
| sensitivity 	| unsigned short	| n/a 			| Defines the step size for analog read. Used to remove analog jitter.

## Errors
If an error occurs after delimiter has been received, all further data is ignored until a LF is reached. This is by design.

| Error message			| Description
| --------------------- | -
| [ ] Aborted 			| Key input was not completed before being canceled
| [ ] No matching key 	| The received key did not match a config item
| Invalid input (%c) 	| Numerical input received non-numerical character
| Value was too high and clamped down to maximum value xxxxx | Integer received a value higher than the maximum value for a 16 bit integer
| Value was too low and clamped up to minimum value of -xxxxx | Signed integer received a value lower than the minimum value for a 16 bit integer
| Maximum input length reached | The text input has exceeded the maximum length for the specified config item

## Tips
* When calibrating the analog input with callow and calhigh, make sure to set sensitivity to 0 before to see fine details in the analog values


## Examples
* To configure the Wi-Fi SSID to `myWifi`, it would look like this `ssid=myWifi\n\n`
* To configure the port to 80, it would look like this `port=80\n\n`
* To configure minimum speed to -10, it would look like this `speedmin=-10\n\n`
* To configure both the port to 80 and minimum speed to -10 in one go, it would look like this `port=80\nspeedmin=-10\n\n`

## Serial communication
For the pre-built example, configuration is done over the serial connection to the micro controller, most likely connected to a computer over USB.
Communicating with the device over serial can be done with the command line.

### Notes
* The echo command might not like the `-e` argument depending on OS. If that is the case, try changing to printf without `-e` argument or something.

### Read:
Reading logs from the device
* OSX: tested
```sh
cat < `ls /dev/cu.usbserial-* | head -1`
```
* Linux: tested
```sh
cat < `ls /dev/ttyUSB* | head -1`
```
Note: If the device is spamming logs or stops the reading right away, try running this command (had this issue in raspbian)
```sh
stty -echo -F `ls /dev/cu.usbserial-* | head -1`
```
* Windows: I dunno

### Write:
Writing configuration data to the device
* OSX: tested
```sh
echo -e 'key=value\n\n' > `ls /dev/cu.usbserial-* | head -1`
```
* Linux: tested
```sh
echo -e 'key=value\n\n' > `ls /dev/ttyUSB* | head -1`
```
* Windows: I dunno

### Interactive:
Makes the command line interface interactive and lets you write your data to the device and see feedback from it right away in the same window.
In this mode, tabs are especially nice to use as delimiter instead of the normal `=`.
* OSX: tested
```sh
screen `ls /dev/cu.usbserial-* | head -1`
```
Exit with Ctrl+a k y
* Linux: untested
```sh
screen `ls /dev/ttyUSB* | head -1`
```
Note: screen command might need to be installed from the package manager
* Windows: I dunno if its even possible :/

### Manually selecting device
For the commands above, it communicates with the first serial device it finds, but sometimes that is not the correct one.
This chapter describes how to list all serial devices and read or write to the one you want.

#### Listing serial devices
* OSX:
```sh
ls /dev/cu.usbserial-*
```
* Linux:
```sh
ls /dev/ttyUSB*
```
* Windows: ?

#### Writing:
Replace "path" with selected device port from [ls](####listing-serial-devices)

* OSX:
```sh
echo -e 'key=value\n\n' > path
```
* Linux:
```sh
echo -e 'key=value\n\n' > path
```
* Windows: ?

#### Reading:
Replace "path" with selected device port from [ls](####listing-serial-devices)

* OSX:
```sh
cat < path
```
* Linux:
```sh
cat < path
```
* Windows: ?
