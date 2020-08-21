idea about multi wifi

# Scroll thingimajigg configuration

## What we have
Currently we have a parser that accepts `key=value\n` text. Spaces before or after the delimiter is currently not trimmed so `key = value` is not going to find the config property for `key`. When it finds a matching key, it then updates it with the following value until it finds a `\n`. If the value is too long, it is going to stop adding more data, but the data up until that point is still accepted and added. Number values are parsed into integers when they are used instead of when they are stored. This might be better to do when storing, but the EEPROM handles bytes and I am not sure how it is going to handle storing and loading of integers since they are longer than 1 byte.

## Security for HTTP configuration
* It is very important to be able to lock down access to configuring the device over HTTP.
* The best way is probably to have some kind of signin, like there is for routers, teradek vidu and so on.
* Since data would then be transmitted back to the server through an HTTP POST request, it would need a cookie or something like that to store the session login.

## Sending HTTP POST data
* A way of sending form data to the server is to use the form attribute `enctype` set to `text/plain`. This would separate every form element with `\r\n` and not encode any special characters.
* To make the existing parser work with post data in text/plain encoding, the only thing that needs to be added is to ignore `\r` characters since it currently stops value handling at a `\n` anyway.
* When writing the data from the http socket to the parse, it has to add the termination at the end since form data only includes `\r\n` between fields and not at the end. But just passing `\n\n` to the parse after all data has been passed would probably be good enough.

## Handling wifi
* A situation where configuring over wifi could be problematic is when a wifi is configured but not accessable to configure over.
* There needs to be a way to configure the device when the wifi is inaccessible but still available.
* A way of handling this kind of situation is to both connect to an existing wifi and create a hotspot. The hotspot would the only be for configuring and the other network for everything else.
* An idea is to have a timer for the hotspot so it is only available for the first 30 seconds unless someone connects to it. This timer would of course be configurable.

## TODO
* Add documentation for how configuration works. If someone would want to configure manually, there should be information to read about how to do so. Include examples and tables for types and max lengths of all properties. I don't like reading instructions in plain text, so try to put as much information as possible in tables or examples for to make it easy to skim.
* Rework storage of numbers to store them as ints instead of strings.
* Ability to configure over HTTP
