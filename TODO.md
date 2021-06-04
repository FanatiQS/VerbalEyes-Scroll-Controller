## TODO:

* Replace "Done" message in updateConfig to something better that indicates changes have been saved

* Continue working on tests for the API

* Try getting https to work
	Now that socket_write function writes uint8_t instead of char, it might work better with defined length.

* Continue working on HTTP POST configuration with captive portals and softAP. Maybe it should be its own branch?

* Update unfinished documentation

* Find a way to add logs like "invalid ssid" or "invalid ssid key" into verbaleyes_network_connected.
	When getting status, it returns incorrect password code until connected.

* Find a better way for getting random seed than to overwrite clock function on ESP since it does not exist. Maybe it needs a verbaleyes_seed function.

* Maybe extract sha1 into its own file.

* LED to indicate what is going on using the built in led during initialisation and during setting config data

* Config parser - support escaped characters
	Used to escape delimiter and add support for `\n` to LF conversion
	Maybe even escape `\n` to make `\\n` to make multiline comments?
	List of escape sequences found here https://www.lix.polytechnique.fr/~liberti/public/computing/prog/c/C/FUNCTIONS/escape.html. Support \n \r \\ \↵ \x30 (hex)

* Config parser - support comments
	Use `#` to indicate comment
	Ignore everything between `#` and LF

* Config parser - trim spaces before and after key and value
	Trims spaces for key validation, storing values and printing
	Trim all spaces before key (so a key can not start with spaces)
	Trim all spaces after key unless there is a match (so a key can end end with spaces)
	Trim one or multiple spaces before value

## LED INDICATION Notes:
   * writing to conf

   * network connecting

   * port outside range			- bad host/port		-	configuration issue and should not occur with configuration software
   * host connection failed		- bad host/port		-	no server at host:port
   * http response timeout		- bad host/port		-	no responce from server
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
	low brightness shine indicates standby
	high brighness shine indicates sending
