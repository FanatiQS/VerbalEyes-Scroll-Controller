# VerbalEyes Scroll Controller - Core Library
The core library can be used to implement the scroll controller on new platforms.
It is written in C and works in C99 or newer as well as C++.



## Usage
The API is built in a way so that the functions only allow the next function to be called if it returns false but is always allowed to move back to any previous function.
For example, the function `verbaleyes_configure` is always allowed to be called, but `verbaleyes_initialize` is only allowed to be called if `verbaleyes_configure` returns false.

### Functions
These are the public functions from the API.

#### verbaleyes_configure
```c
bool verbaleyes_configure(const uint16_t value)
```
This function is used to update the configuration from the outside.
It handles an input string one character at a time.
The input string must conform to the [configuration protocol](#configuration-protocol).
* The argument `value` is any 8-bit character or EOF if there is input data to read.
* The return value is a boolean indicating if it is currently in configuration mode and handling data.
* If it is in configuration mode, it is not allowed to call `verbaleyes_initialize`, `verbaleyes_setspeed` or `verbaleyes_resetoffset` until all data is handled and the configuration mode has been exited (returns false).
* If all data is handled and the function is still in configuration mode, the data was not a complete or a valid configuration instruction. Configuration mode will automatically exit if being open without a successful update for a specified amount of time.
This time can be customised by defining the macro `CONFIGTIMEOUT` for the file `./src/scroll_controller.c`.

#### verbaleyes_initialize
```c
int8_t verbaleyes_initialize()
```
This function is used to connect to a VerbalEyes server and some other configuration setup.
* The return value is an int8_t with one of three states:
	* VERBALEYES_INIT_DONE / false / 0: Everything is connected and working.
	* VERBALEYES_INIT_WORKING / true / 1: It is not connected but is working on it.
	* VERBALEYES_INIT_ERROR / -1: Connecting has failed somehow.
* The return value can be processed as a boolean if there is no need for errors handling as errors will automatically result in a retry.
* If it does not return `VERBALEYES_INIT_DONE / false / 0`, the functions `verbaleyes_setspeed` and `verbaleyes_resetoffset` are not allowed to be called. The function `verbaleyes_configure` is however allowed to be called.
* This function is only allowed to be called if `verbaleyes_configure` returned false.
* Type `int8_t` is the same as `signed char` on most systems.


#### verbaleyes_setspeed
```c
void verbaleyes_setspeed(const uint16_t value)
```
This function sets the scroll speed on the server if it needs to be updated.
* The argument `value` is a reading from an analog input.
* This function is only allowed to be called if both `verbaleyes_configure` and `verbaleyes_initialize` returned false.

#### verbaleyes_resetoffset
```c
void verbaleyes_resetoffset(const bool value)
```
This function resets the scroll position on button press.
* The argument `value` is a reading from a digital input.
* This function is only allowed to be called if both `verbaleyes_configure` and `verbaleyes_initialize` returned false.



### Required function implementations
Just like C requires you to define the function `main`, there are functions that you are required to define for everything to work.
There are 10 functions that are used by the speed controller but not defined.
These functions are related to things like logging, handling persistent data, connecting to the network and reading/writing to sockets.
These actions all depend on the platform you're working with and is the reason they are not implemented by default.
These functions are prototyped in `scroll-controller.h`

#### Configuration
Configuration functions are about handling persistent data.
This data is used for things like wifi, server address and calibration.

###### Read
```c
char verbaleyes_conf_read(const uint16_t addr)
```
* Reads one byte for the persistent data storage.
* Should return the byte currently at the address `addr` in storage.
* Address range is from `0` to `VERBALEYES_CONFIGLEN`.
* Type `uint16_t` is the same as `unsigned short` on most systems.

###### Write
```c
void verbaleyes_conf_write(const uint16_t addr, const char value)
```
* Writes one byte to the persistent data storage.
* Should set byte at address `addr` to `value`.
* Address range is from `0` to `VERBALEYES_CONFIGLEN`.
* Type `uint16_t` is the same as `unsigned short` on most systems.
* Data is allowed to be buffered until `verbaleyes_conf_commit` is called.

###### Commit
```c
void verbaleyes_conf_commit()
```
* If `verbaleyes_conf_write` buffers the data instead of writing it right away, this function would then write all the buffered data to persistent storage.
* When data is not buffered, this function is most likely not required and can be defined as an empty block.
* Buffering is good for flash memory since it can only write data so many times while an actual EEPROM does not benefit from buffering the data.

#### Network
Network function are related to the wireless network connection.

##### Connect
```c
void verbaleyes_network_connect(const char* ssid, const char* ssidkey)
```
* Connects to a network using `ssid` and `ssidkey`.
* This function does not have a return value, instead, the function `verbaleyes_network_connected` is continuously polled to get network status. This allows it to handle configuration while trying to connect to a network and detect when network disconnects.
* If connecting to network is a sync function, `verbaleyes_network_connected` should return 1 for success and -1 for fail since -1 will fail right away while 0 continues waiting for network to be connected.
* If there is already an active network connection, that one should be disconnected and a new one should be established.
* Both `ssid` and `ssidkey` are null terminated character arrays that can include any character sent to the configuration through the `verbaleyes_configure` function.
* Maximum lengths for `ssid` and `ssidkey` can be found [here](#configuration-items).

###### Connected
```c
int8_t verbaleyes_network_connected()
```
* This function returns the state of the network connection.
* It basically returns a boolean with an extra state for if the state has not settled yet.
* Possible return values:
	* VERBALEYES_CONNECT_SUCCESS / true / 1: The network is connected.
	* VERBALEYES_CONNECT_FAIL / false / 0: The network failed to connect.
	* VERBALEYES_CONNECT_WORKING / -1: The network has not connected or failed yet.
* If the network is in a connecting state with the return value being `VERBALEYES_CONNECT_WORKING` for too long (10 seconds), it automatically rejects the network connection and retries. This time can be customised by defining the macro `CONNECTINGTIMEOUT` for the file `./src/scroll_controller.c`.
* Returning any value other than the 3 states defined has undefined behaviour.
* Type `int8_t` is the same as `signed char` on most systems.

#### Socket
Socket functions are related to the socket connection to the server.

###### Connect
```c
void verbaleyes_socket_connect(const char* host, const uint16_t port)
```
* Connects to the host at address `host` on port `port`.
* This function does not have a return value, instead, the function `verbaleyes_socket_connected` is continuously polled to get socket status. This allows it to handle configuration while trying to connect to a socket and detect when socket disconnects.
* If connecting to socket is a sync function, `verbaleyes_socket_connected` should return 1 for success and -1 for fail since -1 will fail right away while 0 continues waiting for socket to be connected.
* If there is already an active socket connection, that one should be disconnected and a new one should be established.
* If there is still buffered data from a previous socket, that data will automatically be flushed before establishing the WebSocket connection.
* The `host` argument is a null terminated character array that can include any character sent to the configuration through the `verbaleyes_configure` function.
* Maximum lengths for `host` can be found [here](#configuration-items).
* The `port` can be any unsigned 16-bit number.
* Type `uint16_t` is the same as `unsigned short` on most systems.

###### Connected
```c
int8_t verbaleyes_socket_connected()
```
* This function returns the state of the socket connection.
* It basically returns a boolean with an extra state for fail.
* Possible return values:
	* VERBALEYES_CONNECT_SUCCESS / true / 1: The socket is connected.
	* VERBALEYES_CONNECT_FAIL / false / 0: The socket failed to connect.
	* VERBALEYES_CONNECT_WORKING / -1: The socket has not connected or failed yet.
* If the socket is in a connecting state with the return value being `VERBALEYES_CONNECT_WORKING` for too long (10 seconds), it automatically rejects the socket connection and retries. This time can be customised by defining the macro `CONNECTINGTIMEOUT` for the file `./src/scroll_controller.c`.
* Returning any value other than the 3 states defined has undefined behaviour.
* Type `int8_t` is the same as `signed char` on most systems.

###### Read
```c
int16_t verbaleyes_socket_read()
```
* Reads one byte from the socket connected to when `verbaleyes_socket_connect` was called.
* Should return the next byte coming from the socket or `EOF` if no data is available.
* It is very important to return `EOF` if no data is available instead of synchronously waiting for data.
* Type `int16_t` is the same as `short` on most systems.

###### Write
```c
void verbaleyes_socket_write(const uint8_t* data, const size_t len)
```
* Writes an array of characters to the socket connected to when `verbaleyes_socket_connect` was called.
* Argument `data` is not always going to be a null terminated string, so using the length argument to ensure that the entire data block is sent is essential.
* The reason for `data` being a `uint8_t*` instead of `char*` is to make it very clear that it is not a null terminated character array.
* Type `uint8_t` is the same as `unsigned char` on most systems.

#### Logging
Logging functions are related to printing the logs.

###### Log
```c
void verbaleyes_log(const char* msg, size_t len)
```
* Logs messages about what is going on and is the best way to know that is going on in the internals.
* The message is a null terminated character array that can include any character sent to the configuration through the `verbaleyes_configure` function.
* The `len` arguments is not required to be used.
* Messages do not always end with newline, so if something like printf is used that buffers messages up to newlines, it needs to be flushed for some messages to not be very delayed, like progress bars and configuration.



## Configuration protocol
The VerbalEyes Scroll Controller uses a simple text base protocol for all configurations and is normally done over USB, but can work with any unidirectional serial stream.
It is one or more key-value pairs ending with an extra line feed, like this `key=value\n\n`.
Multiple configurations are concatenated, like this `key1=value1\nkeyvalue2\n\n`.

* Note that spaces are not trimmed, so `key = value\n` would not work.
* If configuration is opened but never exited, configuration will simply act as if it received an extra line feed after being open for 60 seconds.
This timeout can be configured at compile-time with the C macro `CONFIGTIMEOUT`.
* If an error occurs after delimiter has been received, all further data is ignored until a LF is reached. This is by design.
* An alternative for the `=` delimiter is to use a tab instead.
* If input value is longer than the max length for that key, the configuration system will not add the overflowing data but everything up until max length is reached will be written to persistent storage. This will result in incorrect data in storage if error occurred.
* Comments are supported if the first character is a `#` sign, everything up to next LF will be ignored.
* Keys are case sensitive.
* Configuration system is meant to be used by sending in an entire configuration string at once. Even though it works sending it characters as they are typed, the user experience is sub par with no support for backspace other than aborting and starting over.

### Configuration item types
These are the types for configuration items

| Type 				| Minimum Value | Maximum Value | Description |
| ----------------- | ------------- | ------------- | -
| string 			| n/a 			| n/a 			| A string is a just a plain string
| unsigned short 	| 0 			| 65535 		| An unsigned short is a string representation of a 16 bit unsigned integer.
| signed short 		| -32767 		| 32767 		| A signed short is a string representation of a 16 bit signed integer. A `-` character is used before the number to indicate it is negative.
| percent 			| 0 			| 100 			| A percent value is a string representation of a percentage value not including the `%` sign. Values above 100 are technically possible but have undefined behaviour.

### Configuration items
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

### Examples
* To configure the Wi-Fi SSID to `myWifi`, it would look like this `ssid=myWifi\n\n`
* To configure the port to 80, it would look like this `port=80\n\n`
* To configure minimum speed to -10, it would look like this `speedmin=-10\n\n`
* To combine all the examples above would look like this `ssid=myWifi\nport=80\nspeedmin=-10\n\n`

### Errors
This is a list of all errors that can occur in the configuration parser

| Error message			| Description
| --------------------- | -
| [ ] Aborted 			| Key input was not completed before being canceled
| [ ] No matching key 	| The received key did not match a config item
| Invalid input (%c) 	| Numerical input received non-numerical character
| Value was too high and clamped down to maximum value xxxxx | Integer received a value higher than the maximum value for a 16 bit integer
| Value was too low and clamped up to minimum value of -xxxxx | Signed integer received a value lower than the minimum value for a 16 bit integer
| Maximum input length reached | The text input has exceeded the maximum length for the specified config item
