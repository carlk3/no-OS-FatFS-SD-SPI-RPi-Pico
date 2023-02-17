# no-OS-FatFS-SD-SPI-RPi-Pico

## Simple library for SD Cards on the Pico

At the heart of this library is ChaN's [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html).
It also contains a Serial Peripheral Interface (SPI) SD Card block driver for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
derived from [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html),
and a 4-bit Secure Digital Input Output (SDIO) driver derived from 
[ZuluSCSI-firmware](https://github.com/ZuluSCSI/ZuluSCSI-firmware). 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

## Migration
If you are migrating a project from an SPI-only branch, e.g., `master`, you will have to change the hardware configuration customization. The `sd_card_t` now contains a new object that specifies the configuration of either an SPI interface or an SDIO interface. See the [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration) section below.

## Features:
* Supports multiple SD Cards, all in a common file system
* Supports 4-bit wide SDIO by PIO, or SPI using built in SPI controllers, or both
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Designed to support multiple SDIO buses
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC)
* Plus all the neat features provided by [FatFS](http://elm-chan.org/fsw/ff/00index_e.html)

## Resources Used
* SPI attached cards:
  * One or two Serial Peripheral Interface (SPI) controllers may be used.
  * For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` and enabled.
  * For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
  * For each SD card attached to an SPI controller, a GPIO is needed for CS, and, optionally, another for CD (Card Detect).
* SDIO attached card:
  * A PIO block
  * Two DMA channels claimed with `dma_claim_unused_channel`
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` and enabled.
  * Six GPIOs for signal pins, and, optionally, another for CD (Card Detect). Four pins must be at fixed offsets from D0 (which itself can be anywhere):
    * CLK_gpio = D0_gpio - 2.
    * D1_gpio = D0_gpio + 1;
    * D2_gpio = D0_gpio + 2;
    * D3_gpio = D0_gpio + 3;

SPI and SDIO can share the same DMA IRQ.

Complete `FatFS_SPI_example`, configured for one SPI attached card and one SDIO attached card, release build, as reported by link flag `-Wl,--print-memory-usage`:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      167056 B         2 MB      7.97%
             RAM:       17524 B       256 KB      6.68%
```

## Performance
Writing and reading a file of 64 MiB (67,108,864 byes) of psuedorandom data on a two freshly-formatted 
Silicon Power 3D NAND U1 32GB microSD cards, one on SPI and one on SDIO, release build (using the command `big_file_test bf 64 3`):
* SPI:
  * Writing
    * Elapsed seconds 58.5
    * Transfer rate 1121 KiB/s (1148 kB/s) (9184 kb/s)
  * Reading
    * Elapsed seconds 56.1
    * Transfer rate 1168 KiB/s (1196 kB/s) (9565 kb/s)
* SDIO:
  * Writing
    * Elapsed seconds 20.6
    * Transfer rate 3186 KiB/s (3263 kB/s) (26103 kb/s)
  * Reading
    * Elapsed seconds 15.9
    * Transfer rate 4114 KiB/s (4213 kB/s) (33704 kb/s)

Writing and reading a nearly 4 GiB file:
```
> big_file_test bf 4095 7
Writing...
Elapsed seconds 1360
Transfer rate 3082 KiB/s (3156 kB/s) (25250 kb/s)
Reading...
Elapsed seconds 1047
Transfer rate 4005 KiB/s (4101 kB/s) (32812 kb/s)
> ls
Directory Listing: 0:/
bf [writable file] [size=4293918720]
```

Results from a port of SdFat's `bench`:

SPI:
```
Card size: 31.95 GB (GB = 1E9 bytes)
...
FILE_SIZE_MB = 5
BUF_SIZE = 20480
...
write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
1384.4,21733,14159,14777
1389.9,18609,14150,14723
...
read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
1437.6,15185,14035,14244
1438.4,15008,14046,14238
...
```

SDIO:
```
...
Card size: 31.95 GB (GB = 1E9 bytes)
...
FILE_SIZE_MB = 5
BUF_SIZE = 20480
...
write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
6378.2,13172,2588,3194
6488.7,6725,2597,3145
...
read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
11915.6,2340,1577,1718
11915.6,2172,1579,1716
...
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

![image](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-pinout.svg "Pinout")
<!--
|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |
-->

Please see [here](https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing) for an example wiring table for an SPI attached card and an SDIO attached card on the same Pico. SDIO is pretty demanding electrically. 
You need good, solid wiring, especially for grounds. A printed circuit board with a ground plane would be nice!

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232043568.jpg "Protoboard, top")
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232026240_3.jpg "Protoboard, bottom")


## Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use none, either or both of the Pico's SPIs.
* You can choose to use zero or more PIO SDIO interfaces. [However, currently, the library has only been tested with zero or one.]
I don't know that there's much call for it.
* It's possible to put more than one card on an SDIO bus, but there is currently no support in this library for it.
* For SDIO, data lines D0 - D3 must be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
Furthermore, the CMD signal must be on GPIO D0 GPIO number - 2, modulo 32. (This can be changed in the PIO code.)
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

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20211214_165648888.MP.jpg "Thing Plus")
## Notes about prewired boards with SD card sockets:
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7). SDIO? For sure it could work with one bit SDIO, but I don't know about 4-bit. It looks like it *can* work, depending on what other functions you need on the board.
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well on SPI1. For SDIO, the data lines are consecutive, but in the reverse order! I think that it could be made to work, but you might have to do some bit twiddling. A downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) works on SPI1. Looks fine for 4-bit wide SDIO.
* [Challenger RP2040 SD/RTC](https://ilabs.se/challenger-rp2040-sd-rtc-datasheet/) looks usable for SPI only. 
* Here is one list of RP2040 boards: [earlephilhower/arduino-pico: Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) Only a fraction of them have an SD card socket.

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

### An instance of `sd_card_t` describes the configuration of one SD card socket.
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
* `type` Type of interface: either `SD_IF_SPI` or `SD_IF_SDIO`
* `use_card_detect` Whether or not to use Card Detect
* `card_detect_gpio` GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` What the GPIO read returns when a card is present (Some sockets use active high, some low)

### An instance of `sd_sdio_t` describes the configuration of one SDIO to SD card interface.
```
typedef struct sd_sdio_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1
//...
} sd_sdio_t;
```
Pins `CLK_gpio`, `D1_gpio`, `D2_gpio`, and `D3_gpio` are at offsets from pin `D0_gpio`.
The offsets are determined by `sd_driver\SDIO\rp2040_sdio.pio`.
  As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30,
    which is -2 in mod32 arithmetic, so:
    
  * CLK_gpio = D0_gpio - 2
  * D1_gpio = D0_gpio + 1
  * D2_gpio = D0_gpio + 2
  * D3_gpio = D0_gpio + 3 

These pin assignments are set implicitly and must not be set explicitly.
* `CLK_gpio` RP2040 GPIO to use for Clock (CLK).
Implicitly set to `(D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32` where `SDIO_CLK_PIN_D0_OFFSET` is defined in `sd_driver/SDIO/rp2040_sdio.pio`.
As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30, which is -2 in mod32 arithmetic, so:
  * CLK_gpio = D0_gpio - 2
* `CMD_gpio` RP2040 GPIO to use for Command/Response (CMD)
* `D0_gpio` RP2040 GPIO to use for Data Line [Bit 0]. The PIO code requires D0 - D3 to be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
* `D1_gpio` RP2040 GPIO to use for Data Line [Bit 1]. Implicitly set to D0_gpio + 1.
* `D2_gpio` RP2040 GPIO to use for Data Line [Bit 2]. Implicitly set to D0_gpio + 2.
* `D3_gpio` RP2040 GPIO to use for Card Detect/Data Line [Bit 3]. Implicitly set to D0_gpio + 3.
* `SDIO_PIO` Which PIO block to use. Defaults to `pio0`. Can be changed to avoid conflicts.
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to `DMA_IRQ_0`. The handler is added with `irq_add_shared_handler`, so it's not exclusive. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.

### An instance of `sd_spi_t` describes the configuration of one SPI to SD card interface.
```
typedef struct sd_spi_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs.
    // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, 
    // GPIO_DRIVE_STRENGTH_8MA = 2, GPIO_DRIVE_STRENGTH_12MA = 3 }
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_t;
```
* `spi` Points to the instance of `spi_t` that is to be used as the SPI to drive the interface for this card
* `ss_gpio` Slave Select (or Chip Select [CS]) for this SD card
* `set_drive_strength` Whether or not to set the drive strength
* `ss_gpio_drive_strength` Drive strength for the SS (or CS)

### An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```
typedef struct {
    // SPI HW
    spi_inst_t *hw_inst;
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;
    uint DMA_IRQ_num; // DMA_IRQ_0 or DMA_IRQ_1

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
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. The handler is added with `irq_add_shared_handler`, so it's not exclusive. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.

### You must provide a definition for the functions declared in `sd_driver/hw_config.h`:  
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

## Using the Application Programming Interface
After `stdio_init_all()`, `time_init()`, and whatever other Pico SDK initialization is required, 
you may call `sd_init_driver()` to initialize the block device driver. `sd_init_driver()` is now[^2] called implicitly by `disk_initialize`, but you might want to call it sooner so that the GPIOs get configured, e.g., if you want to set up a Card Detect interrupt.
* Now, you can start using the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html). Typically,
  * f_mount - Register/Unregister the work area of the volume
  * f_open - Open/Create a file
  * f_write - Write data to the file
  * f_read - Read data from the file
  * f_close - Close an open file
  * f_unmount
* There is a simple example in the `simple_example` subdirectory.
* There is also POSIX-like API wrapper layer in `ff_stdio.h` and `ff_stdio.c`, written for compatibility with [FreeRTOS+FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html) (mainly so that I could reuse some tests from that environment.)

## C++ Wrapper
At heart, this is a C library, but I have made a (thin) C++ wrapper for it: `include\FatFsSd.h`.
It uses "Dynamic Configuration", so the configuration is built at run time.
For most of the functions, refer to [FatFs - Generic FAT Filesystem Module, "Application Interface"](http://elm-chan.org/fsw/ff/00index_e.html).

### namespace FatFsNs
The C++ API is in namespace FatFsNs, to avoid name clashes with other packages (e.g.: SdFat).

### class FatFs
This is a pure static class that represents the global file system as a whole. 
It stores the hardware configuration objects internally, 
which is useful in Arduino-like environments where you have a `setup` and a `loop`.
The objects can be constructed in the `setup` and added to `FatFs` and the copies won't go out of scope
when control leaves `setup` and goes to `loop`. 
It automatically provides these functions required internally by the library:

* `size_t spi_get_num()` Returns the number of SPIs to use  
* `spi_t *spi_get_by_num(size_t num)` Returns a pointer to the SPI "object" at the given (zero origin) index  
* `size_t sd_get_num()` Returns the number of SD cards  
* `sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

Static Public Member Functions:
* `static spi_handle_t add_spi(SpiCfg& Spi)` Use this to add an SPI controller configuration. Returns a handle that can be used to construct an `SdCardSpiCfg`.
* `static SdCard* add_sd_card(SdCard& SdCard)` Use this to add a `SdCardSdioCfg` or `SdCardSpiCfg` to the configuration. Returns a pointer that can be used to access the newly created SDCard object.
* `static size_t        SdCard_get_num ()`
* `static SdCard *SdCard_get_by_num (size_t num)`
* `static SdCard *SdCard_get_by_name (const char *const name)`
* `static size_t        Spi_get_num ()`
* `static Spi    *Spi_get_by_num (size_t num)`
* `static FRESULT       chdrive (const TCHAR *path)`
* `static FRESULT       setcp (WORD cp)`
* `static bool  begin ()`

### class SdCard
Represents an SD card socket. It is generalized: the SD card can be either SPI or SDIO attached.

Public Member Functions:
* `const char * get_name ()`
* `FRESULT      mount ()`
* `FRESULT      unmount ()`
* `FRESULT      format ()`
* `bool         readCID (cid_t *cid)`
* `FATFS *      fatfs ()`
* `uint64_t     get_num_sectors ()`

Static Public Member Functions
* `static FRESULT getfree (const TCHAR *path, DWORD *nclst, FATFS **fatfs)`
* `static FRESULT getlabel (const TCHAR *path, TCHAR *label, DWORD *vsn)`
* `static FRESULT setlabel (const TCHAR *label)`
* `static FRESULT mkfs (const TCHAR *path, const MKFS_PARM *opt, void *work, UINT len)`
* `static FRESULT fdisk (BYTE pdrv, const LBA_t ptbl[], void *work)`

### class SdCardSdioCfg
This is a subclass of `SdCard` that is specific to SDIO-attached cards. 
It only has a constructor. 
Before an instance can be used, it needs to be added to the file system configuration with `FatFs::add_sd_card`.
Then, it can be used as a generic `SdCard`.

```
SdCardSdio::SdCardSdio	(	
  const char * 	pcName,
  uint 	CMD_gpio,
  uint 	D0_gpio,
  bool 	use_card_detect = false,
  uint 	card_detect_gpio = 0,
  uint 	card_detected_true = 0,
  PIO 	SDIO_PIO = pio0,
  uint 	DMA_IRQ_num = DMA_IRQ_0 
)		
```

### class Spi
This represents the configuration of one of the RP2040's SPI controllers.
```
Spi::Spi	(	spi_inst_t * 	hw_inst,
uint 	miso_gpio,
uint 	mosi_gpio,
uint 	sck_gpio,
uint 	baud_rate = 25 * 1000 * 1000,
uint 	DMA_IRQ_num = DMA_IRQ_0,
bool 	set_drive_strength = false,
enum gpio_drive_strength 	mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
enum gpio_drive_strength 	sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA 
)	
```	

### class SdCardSpiCfg
This is a subclass of `SdCard` that is specific to SPI-attached cards. 
It only has a constructor. 
Before an instance can be used, it needs to be added to the file system configuration with `FatFs::add_sd_card`.
Then, it can be used as a generic `SdCard`.

The constructor requires a handle to an `Spi`, 
since there may be more than one SPI,
and multiple SD cards can share one SPI.

```
SdCardSpi::SdCardSpi	(	
  spi_handle_t 	spi_p,
  const char * 	pcName,
  uint 	ss_gpio,
  bool 	use_card_detect = false,
  uint 	card_detect_gpio = 0,
  uint 	card_detected_true = false,
  bool 	set_drive_strength = false,
  enum gpio_drive_strength 	ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA 
)		
```

### class File

 * `FRESULT      open (const TCHAR *path, BYTE mode)`
 * `FRESULT      close ()`
 * `FRESULT      read (void *buff, UINT btr, UINT *br)`
 * `FRESULT      write (const void *buff, UINT btw, UINT *bw)`
 * `FRESULT      lseek (FSIZE_t ofs)`
 * `FRESULT      truncate ()`
 * `FRESULT      sync ()`
 * `int  putc (TCHAR c)`
 * `int  puts (const TCHAR *str)`
 * `int  printf (const TCHAR *str,...)`
 * `TCHAR *      gets (TCHAR *buff, int len)`
 * `bool         eof ()`
 * `BYTE         error ()`
 * `FSIZE_t      tell ()`
 * `FSIZE_t      size ()`
 * `FRESULT      rewind ()`
 * `FRESULT      forward (UINT(*func)(const BYTE *, UINT), UINT btf, UINT *bf)`
 * `FRESULT      expand (FSIZE_t fsz, BYTE opt)`

### class Dir 

Public Member Functions:
* `FRESULT      rewinddir ()`
* `FRESULT      rmdir (const TCHAR *path)`
* `FRESULT      opendir (const TCHAR *path)`
* `FRESULT      closedir ()`
* `FRESULT      readdir (FILINFO *fno)`
* `FRESULT      findfirst (FILINFO *fno, const TCHAR *path, const TCHAR *pattern)`
* `FRESULT      findnext (FILINFO *fno)`
Static Public Member Functions:
* `static FRESULT       mkdir (const TCHAR *path)`
* `static FRESULT       unlink (const TCHAR *path)`
* `static FRESULT       rename (const TCHAR *path_old, const TCHAR *path_new)`
* `static FRESULT       stat (const TCHAR *path, FILINFO *fno)`
* `static FRESULT       chmod (const TCHAR *path, BYTE attr, BYTE mask)`
* `static FRESULT       utime (const TCHAR *path, const FILINFO *fno)`
* `static FRESULT       chdir (const TCHAR *path)`
* `static FRESULT       chdrive (const TCHAR *path)`
* `static FRESULT       getcwd (TCHAR *buff, UINT len)`

## PlatformIO Libary
This library is available at https://registry.platformio.org/libraries/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.
It is currently running with 
```
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board_build.core = earlephilhower
```

## Next Steps
* There is a example data logging application in `data_log_demo.c`. 
It can be launched from the `no-OS-FatFS/example` CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

* If you want to use no-OS-FatFS-SD-SPI-RPi-Pico as a library embedded in another project, use something like:
  ```
  git submodule add -b sdio git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  or
  ```
  git submodule add -b sdio https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  
You will need to pick up the library in CMakeLists.txt:
```
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/src build)
target_link_libraries(_my_app_ src)
```
and `#include "ff.h"`.

Happy hacking!

## Future Directions
You are welcome to contribute to this project! Just submit a Pull Request. Here are some ideas for future enhancements:
* Battery saving: at least stop the SDIO clock when it is not needed
* Support 1-bit SDIO
* Try multiple cards on a single SDIO bus
* Multiple SDIO buses?
* ~~PlatformIO library~~ Done: See https://registry.platformio.org/libraries/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.
* Port SDIO driver to [FreeRTOS-FAT-CLI-for-RPi-Pico](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico)

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
        e.g.: cd /dir1

mkdir <path>:
  Make a new directory.
  <path> Specifies the name of the directory to be created.
        e.g.: mkdir /dir1

del_node <path>:
  Remove directory and all of its contents.
  <path> Specifies the name of the directory to be deleted.
        e.g.: del_node /dir1

ls:
  List directory

cat <filename>:
  Type file contents

simple:
  Run simple FS tests

bench [<drive#:>]:
  A simple binary write/read benchmark

big_file_test <pathname> <size in MiB> <seed>:
 Writes random data to file <pathname>.
 Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)
        e.g.: big_file_test bf 1 1
        or: big_file_test big3G-3 3072 3

cdef:
  Create Disk and Example Files
  Expects card to be already formatted and mounted

swcwdt:
 Stdio With CWD Test
Expects card to be already formatted and mounted.
Note: run cdef first!

loop_swcwdt:
 Run Create Disk and Example Files and Stdio With CWD Test in a loop.
Expects card to be already formatted and mounted.
Note: Type any key to quit.

start_logger:
  Start Data Log Demo

stop_logger:
  Stop Data Log Demo

help:
  Shows this command help.

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
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk, PNY, and  Silicon Power.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
* Better yet, go to somwhere like [JLCPCB](https://jlcpcb.com/) and get a printed circuit board!

[^1]: as of [Pull Request #12 Dynamic configuration](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/12) (in response to [Issue #11 Configurable GPIO pins](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/11)), Sep 11, 2021
[^2]: as of [Pull Request #5 Bug in ff_getcwd when FF_VOLUMES < 2](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/5), Aug 13, 2021

[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high." 
However, the Adafruit card has it's own 47 kΩ pull up on CS - Card Detect / Data Line [Bit 3], rendering it useless for Card Detection.
[^4]: [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)
