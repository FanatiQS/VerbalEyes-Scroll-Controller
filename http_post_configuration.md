# VerbalEyes Scroll Controller - Configuration over HTTP
This file documents everything to do with configuring a VerbalEyes Scroll Controller over HTTP.
It is not implemented, so this document is for deciding how to implement it.



## Core parser
It looks like the configuration parser does not need to be modified to allow for this feature.



## HTTP parser Arduino
A way of sending form data to the server is to use the form attribute `enctype` set to `text/plain` in the browser.
This would separate every form element with `\r\n` and not encode any special characters.
When writing the data from the http socket to the configuration parser, it has to add the termination at the end since form data only includes `\r\n` between fields and not at the end. But just passing `\n\n` to `verbaleyes_configure` after all data has been passed would probably be good enough.



## Serving HTML
Should use raw string literals for to import html files.

There is a way to have multiline text in C++.
Just put `const char* html = R"%%(` as the first line and `)%%"` as the last. Where `%%` is configurable to be anything not available in the html file.
Not tested, but might be useful and could put entire HTML in a header?
Might also be better to store the data in a different way with `PROGMEM`, but I have not yet read up on it.
Maybe it would be better to put the HTML in littlefs or something.

While parsing with `verbaleyes_configure`, logs written to `verbaleyes_log` should be written back to the socket until all data has been parsed.
Check that response data sent after an HTTP POST configuration includes all logs until configuration mode is exited.
That way, there is no need to analyse when `Configuration saved` is logged since all prints related to configuration are going to happen while parsing.
It could use chunked to not require buffering the entire HTML response.
The `EPS8266WebServer.h` library has methods for starting and stopping chunked encoding.

### Log page
Could have a log page that only lists the logs from the Scroll Controller.
This would probably use chunked transmission of HTTP response.
Could infinitely receive text/plain but that looks bad.

### HTML responses for requests
#### For GET on /
Respond with root page.
Close socket.

#### For GET on /log
Respond with log page if socket was authenticated.
Otherwise respond with login page.
Close socket on login, otherwise leave open.

#### For GET on /config
Respond with config page if socket was authenticated.
Otherwise respond with login page.
Close socket.

#### For POST on /login
Match post body, respond with config page if match, otherwise respond with 401.
Close socket.

#### For POST on /config
Check authentication, respond with log if authenticated, otherwise respond with 401.
Close on 401, otherwise leave open.

### HTML pages
#### /

#### /login
GET request.
Could redirect to logged in root page?
POST processes login.

#### /config
GET requires to be logged in.
POST processes configuration.

#### /log
GET responds with logs from the devices `verbaleyes_log`.
Requires to be logged in.

#### /dashboard
Some kind of dashboard after being logged in.

#### 404
For no page at path

#### 401

#### 403
Authentication failed

#### 400
Not POST or GET request

### Unsorted old comments
   is client authenticated
	   false	is path root
		   true	res = login page
		   false	is path config or log
			   true	res = 401
			   false	res = 404
	   true	is path root
		   true	res = root page
		   false	is path config page
			   true	res = config page
			   false	is path log page
				   true	res = log page
				   false	res = 404


   GET
	   /
		   ROOT				private			public=/login
	   /login
		   LOGINGET			public
	   /config
		   CONFIGUPDATEGET		private			public=403
	   /log
		   LOG					private			public=403
	   other
		   404					public
   POST
	   /login
		   LOGINPOST			private=/		public=401
	   /config
		   CONFUPDATEPOST		private=/log	public=403
	   other
		   404
   OTHER
	   400



## Security
It is very important to be able to lock down access to configuring the device over HTTP.

The best way is probably to have some kind of signin, like there is for routers, teradek vidu and so on.

Since data would then be transmitted back to the server through an HTTP POST request, it would need a cookie or something like that to store the session login.

Another way to authenticate is to use query parameters instead of a cookie. The pros about this is that it is possible to configure via cURL or anything similar and the only thing that needs to be done is to serve the html file with a changable `action` on the form element.

A simple way of handling security would be to set up a softAP with a key and then create an http server on that APs IP. That would make it only accessible with the APs key. More info here https://arduino.stackexchange.com/questions/67269/create-one-server-in-ap-mode-and-another-in-station-mode.

Another way is to rely on physical acces by requireing a button to be held down or something.



## Wifi

## Soft Access Point
ESP can have both access point and connect to existing network.
It could setup an access point and serve a configuration webpage that uses simple POST request with same structure as serial data.
The issue is security.
USB configuration is good since it requires physical access.
There could be a password required, but that requires setting a password for the device and might be overlooked by most.
It could also not set up the access point if the password is not set up.
Initially, the network config does not work, but if the user wants it, they could enable it by configuring a password over usb.

Using a softAP still has the problem that it requires a password to be set that can not be changed from the configuration system without rewriting parts of the core api.

An approach could be to only enable a softAP without password if button is held down during boot and then, either the softAP is closed if no one connects for 10 seconds or it closes softAP when button is released if no one has connected.
Another way to do it would be to set up a softAP but when receiving HTTP POST requests on softAP or connected network, it would check if button is pressed before handling HTTP data.

A situation where configuring over wifi without softAP could be problematic is when a wifi is configured but not accessable to configure over.
There needs to be a way to configure the device when the wifi is inaccessible but still available.
A way of handling this kind of situation is to both connect to an existing wifi and create a softAP. The AP would only be for configuring and the other network for everything else.
An idea is to have a timer for the AP so it is only available for the first 30 seconds unless someone connects to it.
It is not really safe to just rely on the fact that no one would connect to if within 30 seconds.

Maybe close down the configuration network if no one has connected to it 10 seconds after boot

### Authentication
A traditional way to autenticate is with a login page and a password.
That would require either:
	* A cookie to be set during authentication and checked at configuration later.
	* Fetch api to use authentication header for the HTTP POST request.
	* IP stored when authenticated and then checked the ip is the same at configuration later.
	Issue with using IP is that if configured from behind a router or something, everyone behind that router would be authenticated.
	Could allow only local IPs.
	* Path could be used to, generated random path on authentication and then check at configuration.
	* A hidden first field in the body could be used and set to random string at response to authentication.
	It would be easy to use with cURL as well since it just sends data to a path defined during authentication.

Whenever a POST is received after authentication, no more POST data should be allowed before a new authentication?

### Captive Portal
Captive portals could be used on the softAP since it would only be used to access the HTTP server on the Scroll Controller.
A captive portal could redirects all traffic to the Scroll Controller so the client would not need to know the IP-address or an mDNS name.

A captive portal should be able to be created on the softAP in Arduino with `dnsServer.start(53, "*", WiFi.softAPIP());`

## Web configurator
Add HTTP POST feature to tools/configure_web?
It could use just a simple submit button in the form.
