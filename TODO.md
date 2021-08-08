## TODO:

# Should be fairly easy
* Replace "Done" message in updateConfig to something better that indicates changes have been saved

* Use int_least8_t instead of int8_t to work on systems with smallest sizes larger than 8bits. It has overflow issues though :( int_fast8_t could be used to make it faster maybe. Both these should be safe for number that never go up to 255, aka low repetition loops

* Maybe redefine CONNECTED, CONNECTING and CONNECTIONFAILED as an ENUM and use it for ensureConnection, updateConfig, network_connected and socket_connected. Then return that ENUM to make it clearer that it only allows those 3 states.

* Maybe rename ensureConnection to verbaleyes_initialize and updateConfig to verbaleyes_configure?

* Fix broken links in readmes



# Markdown
* Make separate file for implementation instructions

* Add documentation about only supporting ipv4 and not ipv6. Implementing ipv6 could be done with a buffer argument since returning 128bit ints does not sound like a good way. That could also be an indication if it is using ipv4 or ipv6. If it is returning a value, it should use ipv4, if it is returning 0, it should use ipv6 from the buffer.

* Update unfinished documentation

* Add readmes for tools and test



# Tests
* Continue working on tests for the API

* Create a file for testing verbalEyes-speed-controller implementation. First updateConfig should be called and it should require a string to be sent in over serial or whatever is used.



# Actual features
* Replace static timeout after fail with exponential backoff (retry quicker at first, but do not spam server forever). Should retry be configurable in config?

* Handle something other than auth response as first message (server would only send ping frame before getting any data)

* Handle close and ping after auth

* Get HTTPS to work
	* HTTPS is working, but there is currently no way to distinguish between HTTP and HTTPS hosts.
	* A big issue with HTTPS is fingerprints. They don't lookup Certificate Authority things in ESP code.

* Continue working on HTTP POST configuration with captive portals and softAP. Maybe it should be its own branch?

* LED to indicate what is going on using the built in led during initialisation and during setting config data



# Not important
* Find a better way for getting random seed than to overwrite clock function on ESP since it does not exist. Maybe it needs a verbaleyes_seed function.

* Maybe extract sha1 into its own file.







* Config parser - support escaped characters
	I kinda feel like this feature is not needed
	Used to escape delimiter and add support for `\n` to LF conversion
	Maybe even escape `\n` to make `\\n` to make multiline comments?
	List of escape sequences found here https://www.lix.polytechnique.fr/~liberti/public/computing/prog/c/C/FUNCTIONS/escape.html. Support:
	 	* \n to add in LF (this is the one that is actually important)
		* \t to add in tab (not important since '=' is main delimiter)
		* \\ to add in slash (important if implemented since slash is escape character)
		* \↵ to escape next newline (not really important unless wanting to comment out multiple lines, might not even work in comments anyway)
	Since tabs already have an alternative character in '=', it is really only the LF that needs a way to be written in printf without -e. This would probably be easiest to instead split up in multiple writes since printf puts a LF at the end anyway.

* Config parser - trim spaces before and after key and value
	Trims spaces for key validation, storing values and printing
	Trim all spaces before key (so a key can not start with spaces)
	Trim all spaces after key unless there is a match (so a key can end with spaces)
	Trim one or multiple spaces before value
	Does ssid, ssidkey, host or path even allow for leading or trailing spaces if even spaces at all?
	An easier way to do it is to trim ALL spaces in key and (all or one) leading spaces for values



## LED INDICATION Notes:
   * Test return state of ensureConnection for led. Should return failed state longer?
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

   * bad host/port: There might not be a VerbalEyes server at host:port or the server is not acting propperly.


	An idea is to have:
	3 blinks indicating error
	fading indicating connecting mode
	constant blinking on off indicating configuration mode
	periodically blinking light indicates standby
	shining led indicates sending
