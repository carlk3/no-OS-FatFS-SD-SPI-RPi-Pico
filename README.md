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

On a SanDisk Class 4 16 GB card, I have been able to push the SPI baud rate as far as 20,833,333 which increases the transfer speed proportionately (but SDIO would be faster!).

## Prerequisites:
* Raspberry Pi Pico
* Something like the [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1478.JPG "Prototype")

![image](https://www.raspberrypi.org/documentation/rp2040/getting-started/static/64b50c4316a7aefef66290dcdecda8be/Pico-R3-SDK11-Pinout.svg "Pinout")

|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |

## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, you might not care about it if you intend to leave the card in all the time, so you can skip it and save an I/O pin.
* You can choose to use either or both of the Pico's SPIs.
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector (or tristate). It should be pulled up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. It's best to add an external pull up resistor of around 5kΩ to 3.3v. You might get away without one if you only run one SD card and don't push the SPI baud rate too high.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config.c); you can't let the CS float on the unused one. 

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone --recurse-submodules git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git no-OS-FatFS`
* Customize:
  * Tailor `sd_driver/hw_config.c` to match hardware
  * Customize `ff14a/source/ffconf.h` as desired
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

lliot <drive#>:
 !DESTRUCTIVE! Low Level I/O Driver Test
	e.g.: lliot 1

format [<drive#:>]:
  Creates an FAT/exFAT volume on the logical drive.
	e.g.: format 0:

mount [<drive#:>]:
  Register the work area of the volume
	e.g.: mount 0:

unmount <drive#:>:
  Unregister the work area of the volume

chdrive <drive#:>:
  Changes the current directory of the logical drive.
  <path> Specifies the directory to be set as current directory.
	e.g.: chdrive 1:

getfree [<drive#:>]:
  Print the free space on drive

cd <path>:
  Changes the current directory of the logical drive.
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

```
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")

## Troubleshooting
* The first thing to try is lowering the SPI baud rate (see hw_config.c). This will also make it easier to use things like logic analyzers.
* Try another brand of SD card. Some handle the SPI protocol better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 

## Next Steps
There is a example data logging application in `data_log_demo.c`. 
It can be launched from the CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

If you want to use FatFs_SPI as a library embedded in another project, use something like:
  ```
  git submodule add git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  or
  ```
  git submodule add https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  
You will need to pick up the library in CMakeLists.txt:
```
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
target_link_libraries(_my_app_ FatFs_SPI)
```
and `#include "ff.h"`.

Happy hacking!
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_20210322_201928116.jpg "Prototype")

## Appendix: Adding Additional Cards
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) additional Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).

Name|SPI0|GPIO|Pin |SPI|MicroSD 0|MicroSD 1
----|----|----|----|---|---------|---------
CD1||14|19|||CD
CS1||15|20|SS or CS||CS
MISO|RX|16|21|DO|DO|DO
CS0||17|22|SS or CS|CS|CS
SCK|SCK|18|24|SCLK|SCK|SCK
MOSI|TX|19|25|DI|DI|DI
CD0||22|29||CD|
||||||
GND|||18, 23||GND|GND
3v3|||36||3v3|3v3

### Wiring: 
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware:
* `sd_driver/hw_config.c` must be edited to add a new instance to `static sd_card_t sd_cards[]`
* Edit `ff14a/source/ffconf.h`. In particular, `FF_VOLUMES`:
```
#define FF_VOLUMES		2
```
