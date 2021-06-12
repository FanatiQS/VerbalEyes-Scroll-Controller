## TODO:
Important todos: HTTPS, documentation, ssid logs, led indication

* Replace "Done" message in updateConfig to something better that indicates changes have been saved

* Continue working on tests for the API

* Replace all calls to logprintf not using variadic function with verbaleyes_log

* Rewrite ws write to use better vsnprintf method. Create frame first, add length+opcode, then offset start. So create buffer, vsnprintf with max offset, add length and opcode, write with offset.

* Create a file for testing verbalEyes-speed-controller implementation. First updateConfig should be called and it should require a string to be sent in over serial or whatever is used.

* Handle something other than auth response as first message

* Handle close and ping after auth

* Get HTTPS to work
	* HTTPS is working, but there is currently no way to distinguish between HTTP and HTTPS hosts.
	* A big issue with HTTPS is fingerprints. They don't lookup Certificate Authority things in ESP code.

* Continue working on HTTP POST configuration with captive portals and softAP. Maybe it should be its own branch?

* Add documentation about only supporting ipv4 and not ipv6. Implementing ipv6 could be done with a buffer argument since returning 128bit ints does not sound like a good way. That could also be an indication if it is using ipv4 or ipv6. If it is returning a value, it should use ipv4, if it is returning 0, it should use ipv6 from the buffer.

* Update unfinished documentation

* Add readmes for tools and test

* Find a way to add logs like "invalid ssid" or "invalid ssid key" into verbaleyes_network_connected.
	Should there be a code for [ connecting, connected, failed ] and not just connected or not?

* Find a better way for getting random seed than to overwrite clock function on ESP since it does not exist. Maybe it needs a verbaleyes_seed function.

* Maybe extract sha1 into its own file.

* LED to indicate what is going on using the built in led during initialisation and during setting config data

* Config parser - support escaped characters
	Used to escape delimiter and add support for `\n` to LF conversion
	Maybe even escape `\n` to make `\\n` to make multiline comments?
	List of escape sequences found here https://www.lix.polytechnique.fr/~liberti/public/computing/prog/c/C/FUNCTIONS/escape.html. Support:
	 	* \n to add in LF (this is the one that is actually important)
		* \t to add in tab (not important since '=' is main delimiter)
		* \\ to add in slash (important if implemented since slash is escape character)
		* \↵ to escape next newline (not really important unless wanting to comment out multiple lines, might not even work in comments anyway)

* Config parser - support comments
	Use `#` to indicate comment
	Ignore everything between `#` and LF

* Config parser - trim spaces before and after key and value
	Trims spaces for key validation, storing values and printing
	Trim all spaces before key (so a key can not start with spaces)
	Trim all spaces after key unless there is a match (so a key can end end with spaces)
	Trim one or multiple spaces before value
	Does ssid, ssidkey, host or path even allow for leading or trailing spaces if even spaces at all?

## LED INDICATION Notes:
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
	fading indicating connecting
	constant blinking on off indicating configuration mode
	periodically blinking light indicates standby
	shining led indicates sending
