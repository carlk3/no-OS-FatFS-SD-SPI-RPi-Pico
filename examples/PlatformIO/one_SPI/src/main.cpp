/* 
Copyright 2023 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/

#include "FatFsSd_C.h"
//
#include "SerialUART.h"

#define printf Serial1.printf
#define puts Serial1.println

/* ********************************************************************** */

/* This example assumes the following wiring:
    | signal | SPI1 | GPIO | card | Description            | 
    | ------ | ---- | ---- | ---- | ---------------------- |
    | MISO   | RX   | 12   |  DO  | Master In, Slave Out   |
    | CS0    | CSn  | 09   |  CS  | Slave (or Chip) Select |
    | SCK    | SCK  | 14   | SCLK | SPI clock              |
    | MOSI   | TX   | 15   |  DI  | Master Out, Slave In   |
    | CD     |      | 13   |  DET | Card Detect            |
*/

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each SPI.
    {
    .hw_inst = spi1,  // SPI component
    .miso_gpio = 12,  // GPIO number (not Pico pin number)
    .mosi_gpio = 15,
    .sck_gpio = 14,
    .baud_rate = 25 * 1000 * 1000,  // Actual frequency: 20833333.
    .DMA_IRQ_num = DMA_IRQ_0
    }
};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",          // Name used to mount device
        .spi_if = {
            .spi = &spis[0],  // Pointer to the SPI driving this card
            .ss_gpio = 9,     // The SPI slave select GPIO for this SD card
        },
        .use_card_detect = true,
        .card_detect_gpio = 13,  // Card detect
        .card_detected_true = 1  // What the GPIO read returns when a card is
                                 // present.
    }
};
extern "C" size_t sd_get_num() { return count_of(sd_cards); }
extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
extern "C" size_t spi_get_num() { return count_of(spis); }
extern "C" spi_t *spi_get_by_num(size_t num) {
    if (num <= spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* ********************************************************************** */

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    time_init();

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __BKPT(1);
    }
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        for (;;) __BKPT(2);
    }
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
        for (;;) __BKPT(3);
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __BKPT(4);
    }
    f_unmount(pSD->pcName);

    puts("Goodbye, world!");
}
void loop() {}