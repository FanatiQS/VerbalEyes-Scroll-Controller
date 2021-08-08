# Building from source
Since this is a development tool, there are no prebuilds available.
The supported platforms are currently MacOS and Linux with no Windows support right now.
To test everything that requires the "device" to connect to a server, a VerbalEyes server is required.

## Prerequisites
* GCC
* Make

## Building
* Navigate to `tools/emulator`
* Run `make`

# Usage
Launch the executable `./emulator`.


## Interaction
The 4 arrow keys are used to interact with the emulator.

#### Right arrow key
Toggles the network state on or off.
It doesn't matter what ssid and ssidkey you give it.

#### Left arrow key
Toggles that socket state on or off.
The emulator connects to an actual VerbalEyes server, so toggling the socket can result in a connection fail if the socket failed to connect to the server.

### Up/Down arrow keys
This is what emulates the scrolling up or down for speed change.
The emulator only has 32 steps and going outside that range is clamped down.


## Configuration
Configurations are persistent and stored in the executable itself.
Updating the configuration is done just by typing it in.
The standard input is in raw mode and handles the input character by character.
This allows more flexibility for testing but is, as it states in the readme, not the best user experience since there is no backspace support.
If something is miss spelled, the only way is to click enter and try again.
