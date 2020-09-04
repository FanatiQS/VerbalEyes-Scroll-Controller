// Gets elapsed time in milliseconds
unsigned long getTime();

// Gets a random seed to use
unsigned long randomSeed();



// Connects to a WiFi network
void networkConnect(const char ssid[], const char key[]);

// Gets the connection status of the WiFi connection
bool networkStatus();



// Connects the socket to a server
bool socketConnect(const char host[], const short port);

// Gets the connection status of the socket connection
bool socketStatus();

// Closes the open socket
void socketClose();

// Checks if the socket has data to read
bool socketHasData();

// Consumes a single character from the sockets response data buffer
char socketRead();

// Sends a string to the server that the socket is connected to
void socketWrite(const char str[], const int len);



// Checks if serial port has any data read
bool serialHasData();

// Consumes a single character from serial buffer
char serialRead();

//!! Serial.print is still used. Mainly because it prints the ip in connectNetwork and that one is not a string, but also because currently it prints both strings and characters.
void serialWriteString(const char str[]);
void serialWriteChar(const char c);



// Reads a character from address in EEPROM
char confRead(const int addr);

// Writes character to address in EEPROM
void confWrite(const int addr, const char c);

// Commits changes made to EEPROM
void confCommit();



// Sets state of info LED
void infoLED(const bool state);

// Reads analog signal for speed
int readSpeed();

// Reads digital signal for a pin
int readButton(const int pin);
