## Usage
This is a script to easily configure a VerbalEyes Scroll Controller.
It can both read logs from the device as well as of course write configurations.
Configurations can either be written directly to a scroll controller or to a preset file.

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
If output to a file, that file can then be used with optino `--path` to write that data to a device.

### Path
Enabled with `-p <path>` or `--path <path>` where path is the file to read configuration from.
This is useful for reading in presets that was created earlier with the `--out` option.

### Eval
Enabled with `-e <data>` or `--eval <data>` where data is the configuration data to add.
The data is a key-value pair and needs to follow the [configuration protocol](../../src/README.md).
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



## Manual configuration
Configurations can be done manually using the command line without any external tools.

#### Listing serial devices
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

#### Writing:
Replace "path" with selected device port from [ls](#listing-serial-devices).

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

#### Reading:
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
