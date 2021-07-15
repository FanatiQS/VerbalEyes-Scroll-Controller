#!/bin/bash

# The first argument is a path to where to output the configuration commands or to serial if undefined
# The second argument prevents delaying writes as done by default to not risk loosing data in transmission

delay=$2

if [[ $# > 0 ]]; then
	path=$1
elif [ $(uname) == "Darwin" ]; then
	path=$(ls /dev/cu.usbserial-*)
else
	path=$(/dev/ttyUSB*)
fi

if [ -z "$path" ]; then
	echo "Unable to get serial interface"
	exit
fi

clearString() {
	key=$1
	for ((len = $2; len >= 0; len--)); do
		printf $key= >> $path
		for ((i = 0; i < len; i++)) {
			printf x >> $path
		}
		if [[ -z "$delay" ]]
		then
			sleep 0.1
		fi
		printf "\\n" >> $path
	done
}

printf "" > $path

clearString ssid 32
clearString ssidkey 63
clearString host 64
clearString path 32
clearString proj 32
clearString projkey 32

printf "port=0\\n" >> $path
printf "speedmin=0\\n" >> $path
printf "speedmax=0\\n" >> $path
printf "deadzone=0\\n" >> $path
printf "callow=0\\n" >> $path
printf "calhigh=0\\n" >> $path
printf "sensitivity=0\\n" >> $path

printf "\\n" >> $path
