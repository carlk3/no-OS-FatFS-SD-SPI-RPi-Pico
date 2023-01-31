# no-OS-FatFS-SD-SPI-RPi-Pico

## Simple library for SD Cards on the Pico

At the heart of this library is ChaN's [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html).
It also contains a Serial Peripheral Interface (SPI) SD Card block driver for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
derived from [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html),
and a 4-bit Secure Digital Input Output (SDIO) driver derived from 
[ZuluSCSI-firmware](https://github.com/ZuluSCSI/ZuluSCSI-firmware) . 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

## Migration
If you are migrating a project from an SPI-only branch, e.g., `master`, you will have to change the hardware configuration customization. The `sd_card_t` now contains a new object the specifies the configuration of either an SPI interface or an SDIO interface. See the [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration) section below.

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1473.JPG "Prototype")

## Features:
* Supports multiple SD Cards, all in a common file system
* Supports 4-bit wide SDIO by PIO, or SPI using built in SPI controllers, or both
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC)
* Plus all the neat features provided by [FatFS](http://elm-chan.org/fsw/ff/00index_e.html)

## Resources Used
* SPI attached cards:
  * One or two Serial Peripheral Interface (SPI) controllers may be used.
  * For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
  * DMA_IRQ_0 is hooked with `irq_add_shared_handler` and enabled.
  * For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
  * For each SD card attached to an SPI controller, a GPIO is needed for CS, and, optionally, another for CD (Card Detect).
* SDIO attached card:
  * A PIO block
  * Two DMA channels
  * A DMA interrupt
  * Six GPIOs (four of which need to be consecutive: D0 - D3) for signal pins, and, optionally, another for CD (Card Detect).

Complete `FatFS_SPI_example`, configured for one SPI attached card and one SDIO attached card, release build, as reported by link flag `-Wl,--print-memory-usage`:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      167056 B         2 MB      7.97%
             RAM:       17524 B       256 KB      6.68%
```

## Performance
Writing and reading a file of 0x10000000 (268,435,456) psuedorandom bytes (1/4 GiB) on a two freshly-formatted SanDisk Class 4 16 GB cards, one on SPI and one on SDIO, release build (using the command `big_file_test bf 0x10000000 7`):
* SPI:
  * Writing
    * Elapsed seconds 244
    * Transfer rate 1077 KiB/s (1103 kB/s)
  * Reading
    * Elapsed seconds 188
    * Transfer rate 1394 KiB/s (1427 kB/s)
* SDIO:
  * Writing
    * Elapsed seconds 103
    * Transfer rate 2542 KiB/s (2603 kB/s)
  * Reading
    * Elapsed seconds 43.5
    * Transfer rate 6023 KiB/s (6168 kB/s) (49340 kb/s)

Results from a port of SdFat's `bench`:
* SPI:
```
Type is FAT32
Card size: 15.93 GB (GB = 1E9 bytes)

Manufacturer ID: 0x3
OEM ID: SD
Product: SC16G
Revision: 8.0
Serial number: 0x9d09cea3
Manufacturing date: 1/2021

FILE_SIZE_MB = 5
BUF_SIZE = 512
Starting write test, please wait.

write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
345.3,92192,1136,1482
349.9,92401,1100,1462

Starting read test, please wait.

read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
922.2,1328,532,553
922.5,1322,532,553

Done
```
* SDIO:
```
Type is FAT32
Card size: 15.93 GB (GB = 1E9 bytes)

Manufacturer ID: 0x3
OEM ID: SD
Product: SC16G
Revision: 8.0
Serial number: 0x9d49ce1d
Manufacturing date: 1/2021

FILE_SIZE_MB = 5
BUF_SIZE = 512
Starting write test, please wait.

write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
489.4,116500,752,1045
397.5,94215,784,1287

Starting read test, please wait.

read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
2108.8,490,230,242
2110.6,489,230,242

Done
```

## Choosing the Interface Type(s)
The main reason to use SDIO is for the much greater speed that the 4-bit wide interface gets you. 
However, you pay for that in pins. 
SPI can get by with four GPIOs for the first card and one more for each additional card.
SDIO needs at least six GPIOs, and the 4 bits of the data bus have to be on consecutive GPIOs.
It is possible to put more than one card on an SDIO bus (each card has an address in the protocol), but at the higher speeds (higher than this implementation can do) the tight timing requirements don't allow it. I haven't tried it.

One strategy: use SDIO for cache and SPI for backing store. 
A similar strategy that I have used: SDIO for fast, interactive use, and SPI to offload data.

## Prerequisites:
* Raspberry Pi Pico
* Something like the [Adafruit Micro SD SPI or SDIO Card Breakout Board](https://www.adafruit.com/product/4682)[^3] or [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~5-10kΩ resistors for pull-ups
* (Optional) A couple of ~100 pF capacitors for decoupling

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

Please see [here](https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing) for an example wiring table for an SPI attached card and an SDIO attached card on the same Pico. SDIO is pretty demanding electrically. 
You need good, solid wiring, especially for grounds. A printed circuit board with a ground plane would be nice!

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230124_160927459.jpg "Protoboard, top")
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230124_160941254.jpg "Protoboard, bottom")


## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use none, either or both of the Pico's SPIs.
* You can choose to use up to one PIO SDIO interface. This limitation could be removed, but since each 4-bit SDIO requires at least six GPIOs,
I don't know that there's much call for it.
* It's possible to put more than one card on an SDIO bus, but there is currently no support in this library for it.
* For SDIO, data lines D0 - D3 must be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors and other electrical considerations
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector (or tristate). It should be pulled up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. It's best to add an external pull up resistor of around 5-50 kΩ to 3.3v. (However, modern cards use strong push pull tristateable outputs. 
On some, you can even configure the card's output drivers using the Driver Stage Register (DSR).[^4]).
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config); you shouldn't let the CS float on the unused one. 
* Driving the SD card directly with the GPIOs is not ideal. Take a look at the CM1624 (https://www.onsemi.com/pdf/datasheet/cm1624-d.pdf). Unfortunately, it's a tiny little surface mount part -- not so easy to work with, but the schematic in the data sheet is still instructive. Besides the pull up resistors, it's probably not a bad idea to have 40 - 100 Ω series terminating resistors at the SD card end of CS, SCK, MISO, MOSI. 
* It can be helpful to add a decoupling capacitor or two (e.g., 10, 100 nF) between 3.3 V and GND on the SD card.
* Note: the [Adafruit Breakout Board](https://learn.adafruit.com/assets/93596) takes care of the pull ups and decoupling caps, but the Sparkfun one doesn't. And, you can never have too many decoupling caps.

### Notes about Card Detect
* There is one case in which Card Detect can be important: when the user can hot swap the physical card while the file system is mounted. In this case, the file system might have no way of knowing that the card was swapped, and so it will continue to assume that its prior knowledge of the FATs and directories is still valid. File system corruption and data loss are the likely results.
* If Card Detect is used, in order to detect a card swap there needs to be a way for the application to be made aware of a change in state when the card is removed. This could take the form of a GPIO interrupt (see [FatFS_SPI_example.cpp](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/blob/master/example/FatFS_SPI_example.cpp)), or polling.
* Some workarounds for absence of Card Detect:
  * If you don't care much about performance or battery life, you could mount the card before each access and unmount it after. This might be a good strategy for a slow data logging application, for example.
  * Some form of polling: if the card is periodically accessed at rate faster than the user can swap cards, then the temporary absence of a card will be noticed, so a swap will be detected. For example, if a data logging application writes a log record to the card once per second, it is unlikely that the user could swap cards between accesses.

## Notes about prewired boards with SD card sockets:
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7). SDIO? For sure it could work with one bit SDIO, but I don't know about 4-bit.
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well, on SPI1. For SDIO, the data lines are consecutive, but in the reverse order! I think that it could be made to work, but you might have to do some bit twiddling. A downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
  * For SparkFun RP2040 Thing Plus:

    |       | SPI0  | GPIO  | Description            | 
    | ----- | ----  | ----- | ---------------------- |
    | MISO  | RX    | 12    | Master In, Slave Out   |
    | CS0   | CSn   | 09    | Slave (or Chip) Select |
    | SCK   | SCK   | 14    | SPI clock              |
    | MOSI  | TX    | 15    | Master Out, Slave In   |
    | CD    |       |       | Card Detect            |
  
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) looks like it could work on SPI1. It has CS on GPIO 15, which is not a pin that the RP2040 built in SPI1 controller would drive as CS, but this driver controls CS explicitly with `gpio_put`, so it doesn't matter. Looks fine for 4-bit wide SDIO.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone -b sdio --recurse-submodules git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git no-OS-FatFS`
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
`sd_card_t` contains an instance of `sd_spi_t` or `sd_sdio_t` to specify the interface used to communicate to the card. 
These objects specify which pins to use for what, SPI baud rate, features like Card Detect, etc.

An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```
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

    // State variables:
// ...
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

An instance of `sd_card_t`describes the configuration of one SD card socket.
```
struct sd_card_t {
    const char *pcName;
    sd_if_t type;
    union {
        sd_spi_t spi_if;
        sd_sdio_t sdio_if;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect

    // Following fields are used to keep track of the state of the card:
//...
}
```
* `pcName` FatFs [Logical Drive](http://elm-chan.org/fsw/ff/doc/filename.html) name (or "number")
* `type` Type of interface: either SD_IF_SPI or SD_IF_SDIO
* `use_card_detect` Whether or not to use Card Detect
* `card_detect_gpio` GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` What the GPIO read returns when a card is present (Some sockets use active high, some low)

An instance of `sd_spi_t`describes the configuration of one SPI to SD card interface.
```
typedef struct sd_spi_t {
    spi_t *spi;
    // Slave select is here in sd_card_t because multiple SDs can share an SPI
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs.
    // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
    // GPIO_DRIVE_STRENGTH_12MA = 3 }
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_t;
```
* `spi` Points to the instance of `spi_t` that is to be used as the SPI to drive the interface for this card
* `ss_gpio` Slave Select (or Chip Select [CS]) for this SD card
* `set_drive_strength` Whether or not to set the drive strength
* `ss_gpio_drive_strength` Drive strength for the SS (or CS)

An instance of `sd_sdio_t`describes the configuration of one SDIO to SD card interface.
```
typedef struct sd_sdio_t {
    uint CLK_gpio;
    uint CMD_gpio;
    uint D0_gpio;
    uint D1_gpio;
    uint D2_gpio;
    uint D3_gpio;
// ...    
} sd_sdio_t;
```
* `CLK_gpio` RP2040 GPIO to use for Clock (CLK). This is a little quirky. It should be set to `SDIO_CLK_GPIO`, which is defined in `sd_driver/SDIO/rp2040_sdio.pio`, and that is where you should specify the GPIO number for the SDIO clock.
* `CMD_gpio` RP2040 GPIO to use for Command/Response (CMD)
* `D0_gpio` RP2040 GPIO to use for Data Line [Bit 0]
* `D1_gpio` RP2040 GPIO to use for Data Line [Bit 1]
* `D2_gpio` RP2040 GPIO to use for Data Line [Bit 2]
* `D3_gpio` RP2040 GPIO to use for Card Detect/Data Line [Bit 3]
The PIO code requires D0 - D3 to be on consecutive GPIOs, with D0 being the lowest numbered GPIO.

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
* One advantage of static configuration is that the fantastic GNU Linker (ld) strips out anything that you don't use.

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")

## Using the Application Programming Interface
After `stdio_init_all();`, `time_init();`, and whatever other Pico SDK initialization is required, you may call `sd_init_driver();` to initialize the SPI block device driver. `sd_init_driver()` is now[^2] called implicitly by `disk_initialize`, but you might want to call it sooner so that the GPIOs get configured, e.g., if you want to set up a Card Detect interrupt.
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
   * For SDIO, you can increase the clock divider in `sd_sdio_begin` in `sd_driver/SDIO/sd_card_sdio.c`.
   ```
    // Increase to 25 MHz clock rate
    rp2040_sdio_init(sd_card_p, 1);
    ```
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk and PNY.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20211214_165648888.MP.jpg)
* Better yet, go to somwhere like [JLCPCB](https://jlcpcb.com/) and get a printed circuit board!

[^1]: as of [Pull Request #12 Dynamic configuration](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/12) (in response to [Issue #11 Configurable GPIO pins](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/11)), Sep 11, 2021
[^2]: as of [Pull Request #5 Bug in ff_getcwd when FF_VOLUMES < 2](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/5), Aug 13, 2021

[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high." 
However, the Adafruit card has it's own 47 kΩ pull up on CS - Card Detect / Data Line [Bit 3], rendering it useless for Card Detection.
[^4]: [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)
