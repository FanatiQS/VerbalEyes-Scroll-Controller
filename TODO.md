# TODO
This todo-list is split into 3 parts defining their priority.
Every priority has categories to split tasks into groups.
The order in the categories are sorted after importance with the most important task first. (Not really up-to-date)
Priority sorting guide:
* Low priority is stuff that is not that important and would be fine if it was never implemented.
* Medium priority is stuff that would be a good feature but not really important until a v1.0.0 release.
* High priority is stuff that should be focused on and should be implemented before a v1.0.0 release.

# TODO [high priority]

## Tools
### Configure Bash
* Include instructions on how to download configure script with curl
	* curl https://raw.githubusercontent.com/FanatiQS/VerbalEyes-ScrollController-Micro/master/tools/configure_bash/configure -o configure; sudo chmod 777 confiure
	* Mention that this is an alternative to downloading the entire repository and that it might be simpler and safer to clone entire repo.



## Markdown
* Add dependency section to src/readme.md that mentions the core requires bearssl and that it is already used in ESPs core.
* Maybe remove documentation about configuring with screen. It is not a good experience and should probably not be used.
* Update board wiring documentation.



## Configure HTTP POST
* Sort new data in http_post_configure.md



## Other
* Filter out old files that are not relevant anymore from `tests2`.



## Tests
* Fill out readme with content
* Write readme for test/readme.md describing how the tests work and what is tested and how to run.
* Mention the test/lib dir and that it automatically downloads bearssl and only saves the required files.
* Mention emulator in test/readme.md and that it can be used for manual testing.
* Add links to test/README.md in or src/readme and mention that the library can be tested.
* Complete test for verbaleyes_initialize
* Create tests for verbaleyes_setspeed and verbaleyes_resetoffset
* Make sure that max length path and host does not exceed the bounds for http request.
* Make sure that whatever the configuration is, the websocket packets does not exceed their bounds.
* All data written to `writeWebSocketFrame` needs to have a length lower than 125 or length byte will be incorrect.
* All calls to logprintf needs to be checked that they are within buffer length.
* Check max http request length is not overflowing.
* Test all configuration reads at max length



## Library
### Refactor
* Maybe remove state to jump to in connectionFailToState argument, it could just clear the 4 LSBs to restart that group. Maybe rename to connectionFailed.

### Debug experience
* Log the opcode when it is not a websocket text frame that is received during connection establishment.



## ESP
* Rename .ino to work when downloaded from github.
* Implement blinky light system for `verbaleyes_setspeed` and `verbaleyes_resetoffset`.
	* A way to indicate transmission is to just turn the led on and let it be turned off by blinky system. It would only be on for the length of the delay though. It would also most likely just turn on again since turning the potentiometer would do multiple sends.
* LED to indicate what is going on using the built in led during initialisation and during setting config data
   * Test return state of verbaleyes_initialize for led. Should return failed state longer?
   * Add return state for button and pot to indicate if it send anything or not
   * writing to conf

   * network connecting

   * port outside range			- bad host/port		-	configuration issue and should not occur with configuration software
   * host connection failed		- bad host/port		-	no server at host:port
   * http response timeout		- bad host/port		-	no response from server
   * websocket response bad		- bad host/port		-	server does not know websocket
   * authentication timeout		- bad host/port		-	server did not send a websocket response
   * websocket format bad		- bad host/port		-	server fucked up websocket frame
   * websocket payload too long	- bad host/port		-	server sent a really long message

   * authentication failed		- bad proj/proj_key	-	no such project or wrong password

   * standby

   * bad host/port: There might not be a VerbalEyes server at host:port or the server is not acting properly.


	An idea is to have:
	3 blinks indicating error
	fading indicating connecting mode
	constant blinking on off indicating configuration mode
	periodically blinking light indicates standby
	shining led indicates sending






# TODO [medium priority]

## Library

### Debug experience
* Maybe print the version of the library to the log? Implementation could just print it during setup if there is no good way to implement it into the core.

### Performance
* Maybe do not rerun mapping calculations after reconnecting to the socket? Make some kind of conditional jump to skip the mapping state if not needed.
* Detect max number of bytes required for log buffer when all logs are gone through. Maybe split speed settings at end of verbaleyes_initialize into multiple log calls to be able to lower buffer length?

### Features
* Autentication fail should be able to know if authentication failed or if it is another type of message that was sent before the authentication response. If it is not an auth response, it should go back to state 10 to parse more websocket messages.
	* Might need an extra state to check if it is an auth response first and then in the next state check if it got authenticated or not.
* Handle authentication error
	* Should the socket be closed and a websocket error code be sent or just a normal close code or nothing?
* Send websocket close frame for both errors in state 10
	* Error code 1002 for a masked frame receive
	* Error code 1009 for too long frame
* Handle close and ping frames received from the server after authentication is complete.
* If first websocket data response is not a response to the request but instead a close frame or a ping frame, it should not just reject it but instead handle it correctly.
	* Is it required to send back the close and ping payload? Could that be a slight simplification of this websocket implementation?
		* The websocket spec mentions that `when sending a close frame in response, the endpoint typically echoes the status code it received`.
		* It does not mention echoing the `reason`.
	* It should restart from state 2 when the close frame is handled and stay at state 9 for a ping frame.
		* Unfortunately, the spec `A Pong frame sent in response to a Ping frame must have identical "Application data" as found in the message body of the Ping frame being replied to.`. That makes it a lot harder to implement since the scroll_controller is built around not expecting all data to be readable all at once.
	* If it is neither a ping or close frame, it should close with error code 1003 (close_unsupported)
	* For every unexpected opcode, it should flush the entire websocket header and payload before continuing.
	* To read payload with the possibility of it being incomplete, it should probably move to a state where it can read in the data while still being able to jump back to where it came from when data has been read.
* Trim spaces when processing configuration keys and values to allow for "key = value" instead of just "key=value".
	Trim spaces before and after key and value
	Trim all spaces before key (so a key can not start with spaces)
	Trim all spaces after key unless there is a match (so a key can end with spaces)
	Trim one or multiple spaces before value
	Does ssid, ssidkey, host or path even allow for leading or trailing spaces if even spaces at all?
	An easier way to do it is to trim ALL spaces in key and (all or one) leading spaces for values
* Maybe make timeout after failed connection use exponential backoff to quickly retry but not bombard the server with retries?

### Test
* Make sure no integers overflow since how that is handled by the processor is an undefined behaviour. (all but setspeed and resetoffset are checked)(scrollOffset can overflow)



## Tools
### Configure Web
* Use tabs for web-serial/shell-gen/http-post in configure_web?
* Not able to click on clear tty button in configure_web since scrollbar is in the way.
* Fix screen shift in configure_web when text fields are expanded? Added for when `reduced motion` is enabled.
* Check and maybe rework configure_web generate shell cmd. Maybe it could get port once? Should there really be something like this now that configure_bash exists?

### Configure Bash
* Rename configurations in configure_bash to not be just a simple executable?
* Maybe add right/left arrow key navigation support?
* Should there be an option to make a script executable like the presets config_calibrate and config_clear instead of always being able to parse anything not starting with a `-` as a path.
* Should there be an option when launched without options to start reading? That would make it possible to read logs without having to use the command line.



## ESP
* Get HTTPS to work
	* HTTPS is working, but there is currently no way to distinguish between HTTP and HTTPS hosts. So it can only work with HTTP or HTTPS, not both.
	* A big issue with HTTPS is fingerprints. They don't lookup Certificate Authority in ESP code. That either has to be figured out or fingerprints have to be ignored.
	* Maybe have something in the core to switch from http client to https. That would probably require another function. Detect it by connecting with http and if it gets a response code to upgrade or something.
	* Maybe a simpler way is to have a unique response code from verbaleyes_initialize (like -2) to indicate it failed due to server expecting https. Maybe have unique codes for every type of error.



## Tests
* Maybe deprecate test_config_clear since it is not really needed. Maybe replace it with a test that prints entire config buffer instead to be usable in more situations? That kind of test should probably be a tool though and not a test.
* Make a combined test that runs all tests and makes sure it did not get any errors. Prints a list of all tests and what tests failed.
* Create a file for testing teleprompter-arduino.ino and other implementations. First verbaleyes_configure should be called and it should require a string to be sent in over serial or whatever is used. That string could contain information like ssid and so on, but can be much stricter than the serial parser in core since the test should require all data be sent at once in a specific order or whatever.



## Markdown
* Document undefined behaviour for speed conf items.
* Mention undefined behaviours for configuration: callow > calhigh, callow and deadzone both very high (migh overflow speedOffset), sensitivity higher than calhigh





# TODO [low priority]

## Library

### Usability
* Make it work as an arduino library, like, one that exists in a library directory and can be added to the library manager.
* Maybe add arduino examples.
* Should the function `verbaleyes_conf_commit` be weakly defined to not require it to be defined if data is saved as it comes in?
	* Doing that would look something like this:
		```
		#if defined __has_attribute
			#if __has_attribute (weak)
				void verbaleyes_conf_commit() __attribute__((weak));
				void verbaleyes_conf_commit() {}
			#endif
		#endif
		```
	* If the function is not needed, it is as simple as just defining it as a noop. That is in fact what is done in this example of how it could work.
* Find a better way for getting random seed than to overwrite clock function on ESP since it does not exist. Maybe it needs a verbaleyes_seed function.

### Refactor
* Maybe extract sha1 into its own file.

### Performance
* Use int_fast8_t instead of int8_t to improve performance on systems where bigger integers are faster?
	* Not until a platform actually has a use for it since esp8266 does not.

### Maybe Features
* Should there be a "polling rate" conf item to configure how often the device can send scroll data?
* Add battery level support and connection strength support to relay through server to other clients. Battery level might require a board like this one: https://hitechchain.se/arduinokompatibel/utvecklingsbord-integrerat-esp8266-och-18650-batteri. Should this be implemented into the core library or just the implementation? If implemented outside core, a new websocket function has to be used since the one in core is static.
* Config parser - support escaped characters
	I kinda feel like this feature is not needed
	Used to escape delimiter and add support for `\n` to LF conversion
	Maybe even escape `\n` to make `\\n` to make multiline comments?
	List of escape sequences found here https://www.lix.polytechnique.fr/~liberti/public/computing/prog/c/C/FUNCTIONS/escape.html. Support:
	 	* \n to add in LF (this is the one that is actually important)
		* \t to add in tab (not important since '=' is main delimiter)
		* \\ to add in slash (important if implemented since slash is escape character)
		* \↵ to escape next newline (not really important unless wanting to comment out multiple lines, might not even work in comments anyway)
	Since tabs already have an alternative character in '=', it is really only the LF that needs a way to be written in echo without -e. This would probably be easiest to instead split up in multiple writes since echo puts a LF at the end anyway.



## ESP
* Look up if esp8266 could use webusb to support popup?
