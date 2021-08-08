# Implementing VerbalEyes Speed Controller



## Functions
The API is built in a way so that the functions only allow the next function to be called if it returns false but is always allowed to move back to any previous function.

### updateConfig
```c
bool updateConfig(const uint16_t input)
```
This is the way to change the configuration from the outside.
It handles an input string one character at a time.
* The argument `input` is any 8-bit character or EOF if there is input / data to read.
* The return value is a boolean indicating if it is currently in configuration mode and handling data.
* If it is in configuration mode, it is not allowed to call `ensureConnection`, `updateSpeed` or `jumpToTop` until all data is handled and the configuration mode has been exited and the function returns false / 0.
* If all data is handled and the function is still in configuration mode, the data was not a complete or a valid configuration instruction. Configuration mode will automatically exit if being open without a successful update for a specified amount of time. This time can be customised by defining the macro `CONFIGTIMEOUT` for the file `verbaleEyes_speed_controller.c`.
* Information about the structure of the input data can be found in the [README](../readme.md).

### ensureConnection
```c
int8_t ensureConnection()
```
* The return value is one of three states:
	* CONNECTED: Everything is connected and working.
	* CONNECTING: It is not connected but is working on it.
	* CONNECTIONFAILED: Connecting has failed somehow.
* The return values CONNECTIONFAILED and CONNECTING does not need to be handled separately and is only available to allow indicating a fail to the outside. This way, CONNECTED can be treated as false(as 0) and both the other values as true(as not 0).
* If it is not CONNECTED, the functions `updateSpeed` and `jumpToTop` are not allowed to be called. The function `updateConfig` is allowed to be called though.

### updateSpeed
```c
void updateSpeed(const uint16_t input)
```

### jumpToTop
```c
void jumpToTop(const bool input)
```





## Required function implementations
Just like C requires you to define the function "main", there are functions that you are required to define for everything to work.
There are 11 functions that are used by the speed controller but not defined.
These functions are for doing things like logging, handling persistent data, connecting to the network, connecting to and reading/writing to the socket.
These actions all depend on the platform your working with and are therefore not implemented by default.


### Configuration
Configuration functions are about handling persistent data.
This data is used for things like wifi, server address and calibration.

##### Read
```c
char verbaleyes_conf_read(const uint16_t addr)
```
* Reads one byte for the persistent data storage.
* Should return the byte currently at the address `addr` in storage.
* Address range is from `0` to `CONFIGLEN`.
* Type `uint16_t` is the same as `unsigned short` on most systems.

##### Write
```c
void verbaleyes_conf_write(const uint16_t addr, const char value)
```
* Writes one byte to the persistent data storage.
* Should set byte at address `addr` to `value`.
* Address range is from `0` to `CONFIGLEN`.
* Type `uint16_t` is the same as `unsigned short` on most systems.
* Data is allowed to be buffered until `verbaleyes_conf_commit` is called.

##### Commit
```c
void verbaleyes_conf_commit()
```
* If `verbaleyes_conf_write` buffers the data instead of writing it right away, this function would then write all the buffered data to persistent storage.
* When data is buffered, this function is most likely not required to be defined.
* Buffering is good for flash memory since it can only write data so many times while an actual EEPROM does not benefit from buffering the data.

### Network

#### Connect
```c
void verbaleyes_network_connect(const char* ssid, const char* ssidkey)
```
* Connects to a network using `ssid` and `ssidkey`.
* This function does not have a return value, instead, the function `verbaleyes_network_connected` is continuously polled to get network status. This allows it to handle configuration while trying to connect to a network and detect when network disconnects.
* If connecting to network is a sync function, `verbaleyes_network_connected` should return 1 for success and -1 for fail since -1 will fail right away while 0 continues waiting for network to be connected.
* If there is already an active network connection, that one should be disconnected and a new one should be established.
* Both `ssid` and `ssidkey` are null terminated character arrays that can include any character sent to the configuration through the `updateConfig` function.
* Maximum lengths for `ssid` and `ssidkey` can be found in [readme](should include link here).

##### Connected
```c
int8_t verbaleyes_network_connected();
```
* This function returns the state of the network connection.
* It basically returns a boolean with an extra state for fail.
* Possible return values:
	* 1: Connected. The network is connected.
	* 0: Connecting. The network has not connected or failed yet.
	* -1: Error. The network failed to connect.
* If the network is in a `connecting` state for too long (10 seconds), it automatically rejects the network connection and retries. This time can be customised by defining the macro `CONNECTINGTIMEOUT` for the file `verbaleEyes_speed_controller.c`.
* Type `int8_t` is the same as `signed char` on most systems.

##### GetIP
```c
uint32_t verbaleyes_network_getip()
```
* Gets the IPv4 address of this device on the current network.
* Since it uses a 32-bit return value, only an IPv4 address works and not IPv6. This is not a big deal since this IP address is only used for logging.
* The 32-bit value represents the 4 groups of 8-bit integers in the IPv4 address in network byte order (big-endian). This is the standard way of representing an IPv4 address. So to convert ip address `a.b.c.d` to a 32-bit integer would consist of doing this `a | b << 8 | c << 16 | d << 24`.
* Type `uint32_t` is the same as `unsigned long` on most systems.

### Socket

##### Connect
```c
void verbaleyes_socket_connect(const char* host, const uint16_t port)
```
* Connects to the host at address `host` on port `port`.
* This function does not have a return value, instead, the function `verbaleyes_socket_connected` is continuously polled to get socket status. This allows it to handle configuration while trying to connect to a socket and detect when socket disconnects.
* If connecting to socket is a sync function, `verbaleyes_socket_connected` should return 1 for success and -1 for fail since -1 will fail right away while 0 continues waiting for socket to be connected.
* If there is already an active socket connection, that one should be disconnected and a new one should be established.
* If there is still buffered data from a previous socket, that data will automatically be flushed before establishing the WebSocket connection.
* The `host` argument is a null terminated character array that can include any character sent to the configuration through the `updateConfig` function.
* Maximum lengths for `host` can be found in [readme](should include link here).
* The `port` can be any unsigned 16-bit number.

##### Connected
```c
int8_t verbaleyes_socket_connected()
```
* This function returns the state of the socket connection.
* It basically returns a boolean with an extra state for fail.
* Possible return values:
	* 1: Connected. The socket is connected.
	* 0: Connecting. The socket has not connected or failed yet.
	* -1: Error. The socket failed to connect.
* If the socket is in a `connecting` state for too long (10 seconds), it automatically rejects the socket connection and retries. This time can be customised by defining the macro `CONNECTINGTIMEOUT` for the file `verbaleEyes_speed_controller.c`.
* Type `int8_t` is the same as `signed char` on most systems.

##### Read
```c
int16_t verbaleyes_socket_read()
```
* Reads one byte from the socket connected to when `verbaleyes_socket_connect` was called.
* Should return the next byte coming from the socket or `EOF` if no data is available.
* It is very important to return `EOF` if no data is available instead of synchronously waiting for data.
* Type `int16_t` is the same as `short` on most systems.

##### Write
```c
void verbaleyes_socket_write(const uint8_t* data, const size_t len)
```
* Writes an array of characters to the socket connected to when `verbaleyes_socket_connect` was called.
* Argument `data` is not always going to be a null terminated string, so using the length argument to ensure that the entire data block is sent is essential.
* The reason for `data` being a `uint8_t*` instead of `char*` is to make it very clear that it is not a null terminated character array.
* Type `uint8_t` is the same as `unsigned char` on most systems.

### Logging

##### Log
```c
void verbaleyes_log(const char* msg, size_t len)
```
* Logs messages about what is going on and is the only way to know that is going wrong when something happens.
* The message is a null terminated character array that can include any character sent to the configuration through the `updateConfig` function.
* The `len` arguments is not required to be used.
* Messages do not always end with newline, so if something like printf is used that buffers messages up to newlines, it needs to be flushed for some messages to not be very delayed, like progress bars and configuration.
