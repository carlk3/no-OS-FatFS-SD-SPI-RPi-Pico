/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*

This file should be tailored to match the hardware design.

There should be one element of the spi[] array for each hardware SPI used.

There should be one element of the sd_cards[] array for each SD card slot.
The name is should correspond to the FatFs "logical drive" identifier.
(See http://elm-chan.org/fsw/ff/doc/filename.html#vol)
The rest of the constants will depend on the type of
socket, which SPI it is driven by, and how it is wired.

*/

#include <string.h>
//
#include "my_debug.h"
//
#include "hw_config.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */

/* This configuration is for the [SparkFun Thing Plus - RP2040](https://www.sparkfun.com/products/17745),
    which has the following hardware configuration:

|                                   | Old  |      |      |                            |                                      |
| Signal                            | term | spi0 | GPIO | Description                | Controller/Peripheral (NEW)          |
| --------------------------------- | ---- | ---- | ---- | -------------------------- | ------------------------------------ |
| DATA 0/CIPO (or Peripheral's SDO) | MISO | RX   | 12   | Master In Slave Out (MISO) | Controller In, Peripheral Out (CIPO) |
| CMD/COPI (or Peripheral's SDI)    | MOSI | TX   | 15   | Master Out Slave In (MOSI) | Controller Out Peripheral In (COPI)  |
| CLK/SCK                           | SCK  | SCK  | 14   | Serial Clock               |                                      |
| DATA 3/CS                         | SS   | CSn  | 9    | Slave Select pin (SS)      | Chip Select Pin (CS)                 |

See [RP2040 Thing Plus Hookup Guide](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview).
*/

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each SPI.
    {
        .hw_inst = spi1,  // SPI component
        .miso_gpio = 12,  // GPIO number (not pin number)
        .mosi_gpio = 15,
        .sck_gpio = 14,

        // .baud_rate = 1000 * 1000
        //.baud_rate = 12500 * 1000
        .baud_rate = 25 * 1000 * 1000   // Actual frequency: 20833333.
    }};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 9      // The SPI slave select GPIO for this SD card

/* ********************************************************************** */
size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
