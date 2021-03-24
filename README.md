# no-OS-FatFS-SD-SPI-RPi-Pico

## Simple library for SD Cards on the Pico

At the heart of this library is ChaN's [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html).
It also contains a Serial Peripheral Interface (SPI) SD Card block driver for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/),
derived from [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html). 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1473.JPG "Prototype")

## Features:
* Supports multiple SD Cards
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC)
* Plus all the neat features provided by [FatFS](http://elm-chan.org/fsw/ff/00index_e.html)

## Performance
Using a Debug build: Writing and reading a file of 0xC0000000 (3,221,225,472) random bytes (3 GiB) on a SanDisk 32GB card with SPI baud rate 12,500,000:
* Writing
  * Elapsed seconds 4113.8
  * Transfer rate 764.7 KiB/s
* Reading (and verifying)
  * Elapsed seconds 3396.9
  * Transfer rate 926.1 KiB/s

I have been able to push the SPI baud rate as far as 20,833,333 which increases the transfer speed proportionately (but SDIO would be faster!).

## Prerequisites:
* Raspberry Pi Pico
* Something like the [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1478.JPG "Prototype")

![image](https://www.raspberrypi.org/documentation/rp2040/getting-started/static/64b50c4316a7aefef66290dcdecda8be/Pico-R3-SDK11-Pinout.svg "Pinout")

|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 |
| ----- | ----  | ----- | ---   | --------  | --------- |
| MOSI  | TX    | 16    | 25    | DI        | DI        |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       |
| MISO  | RX    | 19    | 21    | DO        | DO        |
| CD    |       | 22    | 29    |           | CD        |
| GND   |       |       | 18,23 |           | GND       |
| 3v3   |       |       | 36    |           | 3v3       |

## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* You can choose to use either or both of the Pico's SPIs.
* To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone --recurse-submodules git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git no-OS-FatFS`
* Customize:
  * Tailor `sd_driver/hw_config.c` to match hardware
  * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer
* Build:
```  
   cd no-OS-FatFS
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  
## Operation:
* Connect a terminal. [PuTTY](https://www.putty.org/) or `tio` work OK. For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    > 
```    
* The `help` command describes the available commands:
```    
setrtc <DD> <MM> <YY> <hh> <mm> <ss>:
  Set Real Time Clock
  Parameters: new date (DD MM YY) new time in 24-hour format (hh mm ss)
	e.g.:setrtc 16 3 21 0 4 0

date:
 Print current date and time

lliot <device name>:
 !DESTRUCTIVE! Low Level I/O Driver Test
	e.g.: lliot sd0

format <drive#:>:
  Creates an FAT/exFAT volume on the logical drive.
	e.g.: format 0:

mount <drive#:>:
  Register the work area of the volume
	e.g.: mount 0:

unmount <drive#:>:
  Unregister the work area of the volume

cd <path>:
  Changes the current directory of the logical drive. Also, the current drive can be changed.
  <path> Specifies the directory to be set as current directory.
	e.g.: cd 1:/dir1

mkdir <path>:
  Make a new directory.
  <path> Specifies the name of the directory to be created.
	e.g.: mkdir /dir1

ls:
  List directory

cat <filename>:
  Type file contents

simple:
  Run simple FS tests

big_file_test <pathname> <size in bytes> <seed>:
 Writes random data to file <pathname>.
 <size in bytes> must be multiple of 512.
	e.g.: big_file_test bf 1048576 1
	or: big_file_test big3G-3 0xC0000000 3

cdef:
  Create Disk and Example Files
  Expects card to be already formatted and mounted

start_logger:
  Start Data Log Demo

stop_logger:
  Stop Data Log Demo

help:
  Shows this command help.
```

## Troubleshooting
* Logic analyzer:

## Next Steps
There is a example data logging application in `data_log_demo.c`. 
It can be launched from the CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

Generally, to embed the library in your own application you will need to pick up the library and a dependency or two:
```
target_link_libraries(my_app
    FatFs_SPI
    hardware_clocks
)
```
and the FatFS header file:
```
target_include_directories(my_app PUBLIC
    ff14a/source
)
```
and `#include "ff.h"`.

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")
Happy hacking!

## Appendix: Adding a Second Card
* #define FF_VOLUMES		2 in ff14a/source/ffconf.h

