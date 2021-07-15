#!/bin/bash

# The first argument is a path to where to output the configuration commands or to serial if undefined
# The second argument defines if a delay should be used between writes to not risk loosing data in transmission due to hardware not keeping up. Can use true/false or 1/0. Defaults to true

# Second argument defines if a delay should be used between each configuration, defaults to true
if [[ -z "$2" || $2 -eq 1 || $2 == true ]]; then
	delay=1
fi

# Use path argument if is defined
if [[ $# > 0 ]]; then
	path=$1
# Gets path to serial device for mac
elif [ $(uname) == "Darwin" ]; then
	path=$(ls /dev/cu.usbserial-*)
# Gets path to serial device for linux
else
	path=$(/dev/ttyUSB*)
fi

# Aborts if path is empty
if [ -z "$path" ]; then
	echo "Unable to get serial interface"
	exit
fi

clearString() {
	key=$1

	# Iterate through every length
	for ((len = $2; len >= 0; len--)); do
		# Prints configuration with a value of current length
		printf $key= >> $path
		for ((i = 0; i < len; i++)) {
			printf x >> $path
		}

		# Delay for hardware to keep up if not disabled
		if [ "$delay" ]; then
			sleep 0.1
		fi

		# Exits configuration for current length
		printf "\\n" >> $path
	done
}

# Prints null bytes for every configuration taking a string
clearString ssid 32
clearString ssidkey 63
clearString host 64
clearString path 32
clearString proj 32
clearString projkey 32

# Prints 0 for every configuration taking a number
printf "port=0\\n" >> $path
printf "speedmin=0\\n" >> $path
printf "speedmax=0\\n" >> $path
printf "deadzone=0\\n" >> $path
printf "callow=0\\n" >> $path
printf "calhigh=0\\n" >> $path
printf "sensitivity=0\\n" >> $path

# Exits configuration mode
printf "\\n" >> $path
