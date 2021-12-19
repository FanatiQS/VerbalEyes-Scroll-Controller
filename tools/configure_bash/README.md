# VerbalEyes Scroll Controller - Bash Configurator
This is a script to easily configure a VerbalEyes Scroll Controller.
It can both read logs from the device as well as of course write configurations.
Configurations can either be written directly to a scroll controller or to a preset file.

## Usage
The script has these options available.
All options can be combined.
Defaults to `--form` if `--read`, `--path` or `--eval` were not used.

### Read
Enabled with `-r` or `--read`.
It reads logs from the device.
Can be combined with writing data or used by itself.

### Form
Enabled with `-f` or `--form`.
It opens an interactive form to add fields with a little bit of an interface.
The form is used by default if no paths or eval strings are used.

### Output
Enabled with `-o <path>` or `--out <path>` where path is where to output the data to.
This can be either a serial device or a file path.
Details on finding serial device paths can be found [here](#listing-serial-devices).
If output to a file, that file can then be used with option `--path` to write that data to a device.

### Path
Enabled with `-p <path>` or `--path <path>` where path is the file to read configuration from.
This is useful for reading in presets that was created earlier with the `--out` option.

### Eval
Enabled with `-e <data>` or `--eval <data>` where data is the configuration data to add.
The data is a key-value pair and needs to follow the [configuration protocol](../../src/README.md#configuration-protocol).
That is why this option is not recommended if you are not very familiar with the configuration protocol.

### Sleep
Enabled with `-s <seconds>` or `--sleep <seconds>` where seconds is the number of seconds to sleep.
The sleep option is more a debug option.
It delays writing data to let the reader startup first.
When data is not printed to the console after writing a configuration, it can be useful to tell it to wait 1 second before sending anything by using the option `-s 1`.



## Presets
Presets can be created, either manually or with the `--out` option with the configure script.
They can later be written to a device by using the `--path` option.
Comments are supported with the hash sign.

### Calibrate
TODO: Add calibrate instructions for using config_calibrate preset.

### Clear
This is a simple preset to clear a devices persistent storage.
Useful to remove sensitive data like WiFi credentials.



## Errors
* Options that are not recognised will throw and error.
* If the option `--out` is not used, the script tries to automatically detect a serial device to use.
If unable to, an error will be thrown.
* A list of errors that can occur on the scroll controller when parsing input can be found [here](../../src/README.md#errors).



## Manual configuration
Configurations can be done manually using the command line without any external tools.

### Listing serial devices
Before we can read or write data, we need to know where the serial device can be located.
These commands list all serial devices connected to the computer.
Hopefully there is only 1 serial device connected to make it easier to know which is which.

* OSX:
```sh
ls /dev/cu.usbserial-*
```
* Linux:
```sh
ls /dev/ttyUSB*
```
* Windows:
```ps
?
```

### Writing
Replace "path" with selected device port from [ls](#listing-serial-devices).

The protocol for how the data should be defined is available [here](../../src/README.md#configuration-protocol).

* OSX:
```sh
printf 'key=value\n\n' > path
```
* Linux:
```sh
printf 'key=value\n\n' > path
```
* Windows:
```ps
?
```

### Reading
Replace "path" with selected device port from [ls](#listing-serial-devices)

* OSX:
```sh
cat < path
```
* Linux:
```sh
cat < path
```
* Windows:
```ps
?
```

### Read first device found
Automatically finds the first device and reads from it

* OSX:
```sh
cat < `ls /dev/cu.usbserial-* | head -1`
```
* Linux:
```sh
cat < `ls /dev/ttyUSB* | head -1`
```
* Windows:
```ps
?
```

### Write first device found
The protocol for how the data should be defined is available [here](../../src/README.md#configuration-protocol).

* OSX:
```sh
echo -e 'key=value\n\n' > `ls /dev/cu.usbserial-* | head -1`
```
* Linux:
```sh
echo -e 'key=value\n\n' > `ls /dev/ttyUSB* | head -1`
```
* Windows:
```ps
?
```

### Interactive
This is not a recommended way of working as it is not a great experience, explained in more detail [here](../../src/README.md#configuration-protocol).

Makes the command line interface interactive and lets you write your data to the device and see feedback from it right away in the same window.

The protocol for how the data should be defined is available [here](../../src/README.md#configuration-protocol).

In this mode, tabs are especially nice to use as delimiter instead of the normal `=`.

* OSX:
```sh
screen `ls /dev/cu.usbserial-* | head -1`
```
Exit with Ctrl+a k y
* Linux:
```sh
screen `ls /dev/ttyUSB* | head -1`
```
Note: screen command might need to be installed from the package manager
* Windows:
```ps
I dunno if its even possible :/
```



## Note
* If the device is spamming logs or stops the reading right away, try running this command (had this issue in raspbian).
```sh
stty -echo -F `ls /dev/cu.usbserial-* | head -1`
```
