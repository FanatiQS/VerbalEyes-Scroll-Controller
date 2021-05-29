# Scroll configuration over HTTP
This file documents ideas on how configuration over HTTP could work.

* ESP can have both access point and connect to existing network. It could setup an access point and serve a configuration webpage that uses simple POST request with same structure as serial data. The issue is security. USB configuration is good since it requires physical access. There could be a password required, but that requires setting a password for the device and might be overlooked by most. It could also not set up the access point if the password is not set up. Initially, the network config does not work, but if the user wants it, they could enable it by configuring a password over usb.

## Security for HTTP configuration
* It is very important to be able to lock down access to configuring the device over HTTP.
* The best way is probably to have some kind of signin, like there is for routers, teradek vidu and so on.
* Since data would then be transmitted back to the server through an HTTP POST request, it would need a cookie or something like that to store the session login.
* Another way to authenticate is to use query parameters instead of a cookie. The pros about this is that it is possible to configure via cURL or anything similar and the only thing that needs to be done is to serve the html file with a changable `action` on the form element.
* A simple way of handling security would be to set up a softAP with a key and then create an http server on that APs IP. That would make it only accessible with the APs key. More info here https://arduino.stackexchange.com/questions/67269/create-one-server-in-ap-mode-and-another-in-station-mode.

## Sending HTTP POST data
* A way of sending form data to the server is to use the form attribute `enctype` set to `text/plain`. This would separate every form element with `\r\n` and not encode any special characters.
* To make the existing parser work with post data in text/plain encoding, the only thing that needs to be added is to ignore `\r` characters since it currently stops value handling at a `\n` anyway.
* When writing the data from the http socket to the parse, it has to add the termination at the end since form data only includes `\r\n` between fields and not at the end. But just passing `\n\n` to the parse after all data has been passed would probably be good enough.

## Handling wifi
* A situation where configuring over wifi could be problematic is when a wifi is configured but not accessable to configure over.
* There needs to be a way to configure the device when the wifi is inaccessible but still available.
* A way of handling this kind of situation is to both connect to an existing wifi and create a hotspot. The hotspot would the only be for configuring and the other network for everything else.
* An idea is to have a timer for the hotspot so it is only available for the first 30 seconds unless someone connects to it. This timer would of course be configurable.

## Importing HTML
* Should use raw string literals for to import html files.
* Just put `const char* html = R"%%(` as the first line and `)%%"` as the last. Where `%%` is configurable to be anything not available in the html file.
* Might also be better to store the data in a different way with `PROGMEM`, but I have not yet read up on it.
* The string does also need to end with a null byte, so adding that is also needed.
