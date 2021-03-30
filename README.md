# teleprompter-speedController-arduino
This is code for the ESP-8266 that will update the scroll speed of the teleprompter system I am building. It uses wi-fi to connect to the server and sends its data with websockets. Network and other settings are configurable through the serial port.

## Reading and Writing from the command line:

### Read:
* OSX: tested
```sh
cat < `ls -1 /dev/cu.usbserial-* | head -1`
```
* Linux: untested
```sh
cat < `ls -1 /dev/ttyUSB* | head -1`
```
* Windows: I dunno, please let me know if you do

### Write:
* OSX: tested
```sh
echo "insert data here" > `ls -1 /dev/cu.usbserial-* | head -1`
```
* Linux: untested
```sh
echo "insert data here" > `ls -1 /dev/ttyUSB* | head -1`
```
#### Note:
* Message should end with a LF
* If the message contains spaces, they have to be escaped or the string has to be wrapped in quotes

### Interactive:
Interactive terminal where you can write your data to the device and see feedback from it right away in the same window.
Tab or '=' are used as delimiter between key and value.
* OSX: tested
```sh
screen `ls -1 /dev/cu.usbserial-* | head -1`
```
Exit with Ctrl+a k y
* Linux: untested
```sh
screen `ls -1 /dev/ttyUSB* | head -1`
```
* Windows: I dunno if its even possible :/

### Step-By-Step
#### Get all Serial device paths
* OSX:
```sh
ls /dev/cu.usbserial-*
```
* Linux: `ls /dev/ttyUSB*`
* Windows: ?

#### Writing:
Use a serial device path result from [ls](#get-all-serial-device-paths)

* OSX:
```sh
echo "insert data here" > /dev/cu.usbserial-*
```
* Linux:
```sh
echo ` message ` > /dev/ttyUSB*
```
* Windows: ?

##### Notes:
* Message should end with a LF
* If the message contains spaces, they have to be escaped or the string has to be wrapped in quotes

#### Reading:
Use a serial device path result from [ls](#get-all-serial-device-paths)

* OSX:
```sh
cat < /dev/cu.usbserial-*
```
* Linux:
```sh
cat < /dev/ttyUSB*
```
* Windows: ?

##### Notes:
* If it does not continue to read, try to run `stty -echo -F ` + path (had this issue in raspbian)
