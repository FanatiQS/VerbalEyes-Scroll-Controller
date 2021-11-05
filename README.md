# VerbalEyes SpeedController Core
This is a C library to control scroll speed and scroll position for a VerbalEyes server with a micro controller.
It works by connecting to Wi-Fi and sending the commands to the server over a WebSocket connection.
Configuration of what network and server to use is done with a simple text based protocol.

# ESP8266 Build
This is an implementation of the library that works on the ESP8266.
Configuration is done over the USB connection with more details further down in this readme.

## Wiring
TODO: Add details on how the board should be wired

## LED indicator
The built-in status LED blinks once every 8 seconds whenever the device is connected to a server and is not parsing incoming configuration data.
When it is actively trying to connect to a server or parses configuration data, it blinks rapidly.



# Core Library
The core library should be able to run on anything supporting C99 or C++.
Because of this, the functions to access to Wi-Fi, sockets, persistent storage and logging are not defined.
These have to be implemented to work on the micro controller that it is used on by defining all the required functions the library uses. These functions are prototyped in `./src/scroll-controller.h` and documented together with the available API functions in [./src/README.md](../src/README.md).



# Configuration
* Configuration of the device is done with a simple text based serial protocol consisting of a key-value-pair structure like this `key=value\n` where `\n` has to be encoded as an actual LF character.
* Spaces before or after the key or value are not trimmed out, so `key = value\n` would not be the same as `key=value\n`.
* Multiple configurations can be chained like this `key1=value1\nkey2=value2\n`.
* An empty line indicates configuration is done, so sending an extra `\n` at the end exits configuration mode.
* After 60 seconds, configuration mode will automatically exit. This timeout can be configured at compile-time with a C macro.
* An alternative for the `=` delimiter is to use a tab instead.
* If input value is longer than the max length for that key, the configuration system will not add the overflowing data but everything up until max length is reached will be written to persistent storage. This will result in incorrect data in storage if error occurred.
* Comments are supported if the first character is a `#` sign, everything up to next LF will be ignored.
* Keys are case sensitive.
* Configuration system is meant to be used by sending in an entire configuration string at once. Even though it works sending it characters as they are typed, the user experience is sub par with, no indication that a configuration value has been null terminated until the next key starts being processed or configuration mode is exited, and no support for backspace other than aborting and starting over.

## Configuration item types
These are the types for configuration items
| Type 				| Minimum Value | Maximum Value | Description |
| ----------------- | ------------- | ------------- | -
| string 			| n/a 			| n/a 			| A string is a just a plain string
| unsigned short 	| 0 			| 65535 		| An unsigned short is a string representation of a 16 bit unsigned integer.
| signed short 		| -32767 		| 32767 		| A signed short is a string representation of a 16 bit signed integer. A `-` character is used before the number to indicate it is negative.
| percent 			| 0 			| 100 			| A percent value is a string representation of a percentage value not including the `%` sign. Values above 100 are technically possible but have undefined behaviour.

## Configuration items
These are all the configurations that can be configured
| Key			| Type 				| Max length 	| Description |
| ------------- | ----------------- | ------------- | -
| ssid 			| string			| 32			| The SSID (WiFi) to connect to.
| ssidkey 		| string 			| 63			| The passphrase to the SSID.
| host 			| string 			| 64			| The host (server) to connect to. Can be an IP address or a DNS name.
| port 			| unsigned short 	| n/a			| The port to use when connecting to the host. Usually 80 for HTTP and 443 for HTTPs.
| path 			| string 			| 32			| The path to use on the host. Requires a `/` for root.
| proj 			| string 			| 32 			| The VerbalEyes project to connect to.
| projkey 		| string 			| 32 			| The password to the VerbalEyes project.
| speedmin 		| signed short 		| n/a 			| The speed to send when the potentiometer is turned all the way in one direction.
| speedmax 		| signed short 		| n/a 			| The speed to send when the potentiometer is turned all the way in the other direction.
| deadzone 		| percent 			| n/a 			| The size of the deadzone around the speed value 0 in percentage of entire range. Used to make 0 mark bigger on the potentiometer.
| callow 		| unsigned short 	| n/a 			| The minimum value from the analog read. Used for calibrating potentiometer when it does not give 0 at the limit.
| calhigh 		| unsigned short 	| n/a 			| The maximum value from the analog read. Used for calibrating the maximum value from the potentiometer. Depends on resolution of ADC on micro controller and used for calibrating potentiometer when it does not give max ADC value at the limit.
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
This is not a recommended way of working as it is not a great experience, explained in more detailed earlier.
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
