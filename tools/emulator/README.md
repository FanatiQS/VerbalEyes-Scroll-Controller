## Usage
Build the binary from source and launch it with the command `./emulator`.
Instructions for how to interact with it can be found below.



## Building from source
Since this is a development tool, there are no compiled binaries available.
The supported platforms are currently MacOS and Linux with no Windows support right now.
To test anything that comes after connecting to the host, a server is required.

### Prerequisites
The prerequisites required to build:
* GCC
* Make

### Building
1. Open a terminal window
2. Navigate to `tools/emulator`
3. Run `make`



### Interaction
Interaction is done by typing or pressing any of the 4 arrow keys for special actions.

###### Right arrow key
Toggles the network state on or off.
It doesn't matter what ssid and ssidkey you give it.

###### Left arrow key
Toggles that socket state on or off.
The emulator connects to an actual VerbalEyes server, so toggling the socket can result in a connection fail if the socket failed to connect to the server.

###### Up/Down arrow keys
This is what emulates the scrolling up or down for speed change.
The emulator only has 32 steps and going outside that range is clamped.

##### Configuration
Configurations are persistent and stored in the executable itself.
Updating the configuration is done just by typing it in.
The standard input is in raw mode and handles the input character by character.
This allows more flexibility for testing but is, as it states in the readme, not the best user experience since there is no backspace support.
If something is misspelled, the only way is to terminate the line with enter and try again.
