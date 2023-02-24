# no-OS-FatFS-SD-SPI-RPi-Pico

**Note:** *This branch is SPI only. If you want to use SDIO (and/or SPI), please switch to the `sdio` branch.*

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

## Resources Used
* At least one (depending on configuration) of the two Serial Peripheral Interface (SPI) controllers is used.
* For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
* DMA_IRQ_0 is hooked with `irq_add_shared_handler` and enabled.
* For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
* For each SD card attached to an SPI controller, a GPIO is needed for CS, and, optionally, another for CD (Card Detect).
<!--* `size simple_example.elf`
```
   text	   data	    bss	    dec	    hex	filename
  68428	     44	   4388	  72860	  11c9c	simple_example.elf
```-->

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
* Something like the [Adafruit Micro SD SPI or SDIO Card Breakout Board](https://www.adafruit.com/product/4682)[^3] or [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)

***Warning***: Stay away from Aduino breakout boards like these: [Micro SD Storage Board Micro SD Card Modules](https://smile.amazon.com/gp/product/B07XF4Q1TT/). They are designed for 5 V Arduino signals. They use simple resistor dividers to drop the signal voltage, and will not work with the 3.3 V Raspberry Pi Pico. There's usually no easy way to bypass the resistors. 

Some people have made micro SD card sockets by soldering pins onto old SD Card adapters.

* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~5-10kΩ resistors for pull-ups
* (Optional) A couple of capacitors for decoupling. Ideally, use three capacitors: 10 uF, 0.1 uF (100 nF), and .001 uF (1 nF) on the Vdd line to the of the SD card, located close to the socket. (The Adafruit board already has the 10 uF and 0.1 uF: [Schematic](https://learn.adafruit.com/assets/93596).)

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1478.JPG "Prototype")

![image](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-pinout.svg "Pinout")

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
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use either or both of the Pico's SPIs.
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors and other electrical considerations
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector (or tristate). It should be pulled up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. It's best to add an external pull up resistor of around 5kΩ to 3.3v. You might get away without one if you only run one SD card and don't push the SPI baud rate too high.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config); you can't let the CS float on the unused one. 
* Driving the SD card directly with the GPIOs is not ideal. Take a look at the CM1624 (https://www.onsemi.com/pdf/datasheet/cm1624-d.pdf). Unfortunately, it's a tiny little surface mount part -- not so easy to work with, but the schematic in the data sheet is still instructive. Besides the pull up resistors, it's probably not a bad idea to have 40 - 100 Ω series terminating resistors at the SD card end of CS, SCK, MISO, MOSI. 
* It can be helpful to add a decoupling capacitor or two (e.g., 10, 100 nF) between 3.3 V and GND on the SD card.
* Note: the [Adafruit Breakout Board](https://learn.adafruit.com/assets/93596) takes care of the pull ups and decoupling caps, but the Sparkfun one doesn't.

### Notes about Card Detect
* There is one case in which Card Detect can be important: when the user can hot swap the physical card while the file system is mounted. In this case, the application might have no way of knowing that the card was swapped, and so it will continue to assume that its prior knowledge of the FATs and directories is still valid. File system corruption and data loss are the likely result.
* If Card Detect is used, in order to detect a card swap there needs to be a way for the application to be made aware of a change in state when the card is removed. This could take the form of a GPIO interrupt (see [FatFS_SPI_example.cpp](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/blob/master/example/FatFS_SPI_example.cpp)), or polling.
* Some workarounds:
  * If you don't care much about performance or battery life, you could mount the card before each access and unmount it after. This might be a good strategy for a slow data logging application, for example.
  * Some form of polling: if the card is periodically accessed at rate faster than the user can swap cards, then the temporary absence of a card will be noticed, so a swap will be detected. For example, if a data logging application writes a log record to the card once per second, it is unlikely that the user could swap cards between accesses.

## Notes about prewired boards with SD card sockets:
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7).
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well, on SPI1. The only downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
  * For SparkFun RP2040 Thing Plus:

    |       | SPI0  | GPIO  | Description            | 
    | ----- | ----  | ----- | ---------------------- |
    | MISO  | RX    | 12    | Master In, Slave Out   |
    | CS0   | CSn   | 09    | Slave (or Chip) Select |
    | SCK   | SCK   | 14    | SPI clock              |
    | MOSI  | TX    | 15    | Master Out, Slave In   |
    | CD    |       |       | Card Detect            |
  
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) looks like it could work on SPI1. It has CS on GPIO 15, which is not a pin that the RP2040 built in SPI1 controller would drive as CS, but this driver controls CS explicitly with `gpio_put`, so it doesn't matter.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone --recurse-submodules git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git no-OS-FatFS`
* Customize:
  * Configure the code to match the hardware: see section [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration), below.
  * Customize `ff14a/source/ffconf.h` as desired
  * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer. 
(See *4.1. Serial input and output on Raspberry Pi Pico* in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) and *2.7.1. Standard Input/Output (stdio) Support* in [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf).) 
* Build:
```  
   cd no-OS-FatFS/example
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  
## Customizing for the Hardware Configuration 
This library can support many different hardware configurations. 
Therefore, the hardware configuration is not defined in the library[^1]. 
Instead, the application must provide it. 
The configuration is defined in "objects" of type `spi_t` (see `sd_driver/spi.h`) and `sd_card_t` (see `sd_driver/sd_card.h`). 
There can be one or more objects of both types.
These objects specify which pins to use for what, SPI baud rate, features like Card Detect, etc.

### An instance of `sd_card_t` describes the configuration of one SD card socket.
```
// "Class" representing SD Cards
struct sd_card_t {
    const char *pcName;
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect
    // Drive strength levels for GPIO outputs.
    // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
    // GPIO_DRIVE_STRENGTH_12MA = 3 }
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
//...
};
```
* `pcName` FatFs [Logical Drive](http://elm-chan.org/fsw/ff/doc/filename.html) name (or "number")
* `ss_gpio` Slave Select (or Chip Select [CS]) for this SD card
* `use_card_detect` Whether or not to use Card Detect
* `card_detect_gpio` GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` What the GPIO read returns when a card is present (Some sockets use active high, some low)
* `set_drive_strength` Whether or not to set the drive strength
* `ss_gpio_drive_strength` Drive strength for the SS (or CS)

### An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```
// "Class" representing SPIs
typedef struct {
    // SPI HW
    spi_inst_t *hw_inst;
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;

    // Drive strength levels for GPIO outputs.
    // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
    // GPIO_DRIVE_STRENGTH_12MA = 3 }
    bool set_drive_strength;
    enum gpio_drive_strength mosi_gpio_drive_strength;
    enum gpio_drive_strength sck_gpio_drive_strength;
//...
} spi_t;

```
* `hw_inst` Identifier for the hardware SPI instance (for use in SPI functions). e.g. `spi0`, `spi1`
* `miso_gpio` SPI Master In, Slave Out (MISO) GPIO number (not Pico pin number). This is connected to the card's Data In (DI).
* `mosi_gpio` SPI Master Out, Slave In (MOSI) GPIO number. This is connected to the card's Data Out (DO).
* `sck_gpio` SPI Serial Clock GPIO number. This is connected to the card's Serial Clock (SCK).
* `baud_rate` Frequency of the SPI Serial Clock
* `set_drive_strength` Specifies whether or not to set the RP2040 GPIO drive strength
* `mosi_gpio_drive_strength` SPI Master Out, Slave In (MOSI) drive strength
* `sck_gpio_drive_strength` SPI Serial Clock (SCK) drive strength

You must provide a definition for the functions declared in `sd_driver/hw_config.h`:  
`size_t spi_get_num()` Returns the number of SPIs to use  
`spi_t *spi_get_by_num(size_t num)` Returns a pointer to the SPI "object" at the given (zero origin) index  
`size_t sd_get_num()` Returns the number of SD cards  
`sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

### Static vs. Dynamic Configuration
The definition of the hardware configuration can either be built in at build time, which I'm calling "static configuration", or supplied at run time, which I call "dynamic configuration". 
In either case, the application simply provides an implementation of the functions declared in `sd_driver/hw_config.h`. 
* See `simple_example.dir/hw_config.c` or `example/hw_config.c` for examples of static configuration.
* See `dynamic_config_example/hw_config.cpp` for an example of dynamic configuration.

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")

## Using the Application Programming Interface
<strike>After `stdio_init_all();`, `time_init();`, and whatever other Pico SDK initialization is required, call `sd_init_driver();` to initialize the SPI block device driver.</strike> \[sd_init_driver() is now[^2] called implicitly.\]
* Now, you can start using the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html). Typically,
  * f_mount - Register/Unregister the work area of the volume
  * f_open - Open/Create a file
  * f_write - Write data to the file
  * f_read - Read data from the file
  * f_close - Close an open file
  * f_unmount
    * There is a simple example in the `simple_example` subdirectory.
* There is also POSIX-like API wrapper layer in `ff_stdio.h` and `ff_stdio.c`, written for compatibility with [FreeRTOS+FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html) (mainly so that I could reuse some tests from that environment.)

## Next Steps
* There is a example data logging application in `data_log_demo.c`. 
It can be launched from the `no-OS-FatFS/example` CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

* If you want to use FatFs_SPI as a library embedded in another project, use something like:
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

## Appendix A: Adding Additional Cards
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) additional Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).

Name|SPI0|GPIO|Pin |SPI|SDIO|MicroSD 0|MicroSD 1
----|----|----|----|---|----|---------|---------
CD1||14|19||||CD
CS1||15|20|SS or CS|DAT3||CS
MISO|RX|16|21|DO|DAT0|DO|DO
CS0||17|22|SS or CS|DAT3|CS|
SCK|SCK|18|24|SCLK|CLK|SCK|SCK
MOSI|TX|19|25|DI|CMD|DI|DI
CD0||22|29|||CD|
|||||||
GND|||18, 23|||GND|GND
3v3|||36|||3v3|3v3

### Wiring: 
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware:
* `hw_config.c` (or equivalent) must be edited to add a new instance to `static sd_card_t sd_cards[]`
* Edit `ff14a/source/ffconf.h`. In particular, `FF_VOLUMES`:
```
#define FF_VOLUMES		2
```

## Appendix B: Operation of `no-OS-FatFS/example`:
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

## Appendix C: Troubleshooting
* The first thing to try is lowering the SPI baud rate (see hw_config.c). This will also make it easier to use things like logic analyzers.
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20211214_165648888.MP.jpg)


[^1]: as of [Pull Request #12 Dynamic configuration](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/12) (in response to [Issue #11 Configurable GPIO pins](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/11)), Sep 11, 2021
[^2]: as of [Pull Request #5 Bug in ff_getcwd when FF_VOLUMES < 2](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/5), Aug 13, 2021
[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high." However, the Adafruit card has it's own 47 kΩ pull up on CS, rendering it useless for Card Detection.
