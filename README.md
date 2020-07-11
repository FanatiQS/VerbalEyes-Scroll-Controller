# teleprompter-speedController-arduino
This is code for the ESP-8266 that will update the scroll speed of the teleprompter system I am building. It uses wi-fi to connect to the server and sends its data with websockets. Network and other settings are configurable through the serial port.

## Reading and Writing from the command line:
### Get the tty path
* OSX: `ls /dev/cu.usbserial-*`
* Linux: `ls /dev/ttyUSB*`
* Windows: ?

### Writing:
* OSX: `echo ` message ` > /dev/cu.usbserial-*`
* Linux: `echo ` message ` > /dev/ttyUSB*`
* Windows: ?
#### Notes:
* Message should end with a LF
* If the message contains spaces, they have to be escaped or the string has to be in quotes

### Reading:
* OSX: `cat > /dev/cu.usbserial-*`
* Linux: `cat /dev/ttyUSB*`
* Windows: ?
#### Notes:
* If it does not continue to read, try to run `stty -echo -F ` + path (had this issue in raspbian)
