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

#include "FatFs.h"
#include "SerialUART.h"

#define printf Serial1.printf
#define puts Serial1.println

/* ********************************************************************** */

// First (if s is not NULL and *s is not a null byte ('\0')) the argument string s is printed, 
//   followed by a colon and a blank. Then the FRESULT error message and a new-line.
static void chk_result(const char *s, FRESULT fr) {
    if (FR_OK != fr) {
        if (s && *s)
            printf("%s: %s (%d)\n", s, FRESULT_str(fr), fr);
        else
            printf("%s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __breakpoint();
    }
}

static void test(FatFs_SdCard *FatFs_SdCard_p) {
    FRESULT fr;

    fr = FatFs_SdCard_p->mount();
    chk_result("mount", fr);
    fr = FatFs::chdrive(FatFs_SdCard_p->get_name());
    chk_result("chdrive", fr);

    const char *const filename = "filename.txt";
    FatFs_File file;
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    chk_result("open", fr);

    // if (f_printf(&fil, "Hello, world!\n") < 0) {
    //     printf("f_printf failed\n");
    //     for (;;) __BKPT(4);
    // }
    fr = file.close();
    chk_result("close", fr);

    FatFs_SdCard_p->unmount();
}

/* ********************************************************************** */
/*
This example assumes the following wiring for SD card 0:

    | signal | SPI1 | GPIO | card | Description            |
    | ------ | ---- | ---- | ---- | ---------------------- |
    | MISO   | RX   | 12   |  DO  | Master In, Slave Out   |
    | CS0    | CSn  | 09   |  CS  | Slave (or Chip) Select |
    | SCK    | SCK  | 14   | SCLK | SPI clock              |
    | MOSI   | TX   | 15   |  DI  | Master Out, Slave In   |
    | CD     |      | 13   |  DET | Card Detect            |

This example assumes the following wiring for SD card 1:

    | GPIO  |  card | Function    |
    | ----  |  ---- | ----------- |
    |  16   |  DET  | Card Detect |
    |  17   |  CLK  | SDIO_CLK    |
    |  18   |  CMD  | SDIO_CMD    |
    |  19   |  DAT0 | SDIO_D0     |
    |  20   |  DAT1 | SDIO_D1     |
    |  21   |  DAT2 | SDIO_D2     |
    |  22   |  DAT3 | SDIO_D3     |

Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
    CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
    As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
        which is -2 in mod32 arithmetic, so:
    CLK_gpio = D0_gpio - 2
    D1_gpio = D0_gpio + 1
    D2_gpio = D0_gpio + 2
    D3_gpio = D0_gpio + 3
*/

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    puts("Hello, world!");

    /* Hardware Configuration of SPI object */

    // GPIO numbers, not Pico pin numbers!
    FatFs_Spi spi(
        spi1,  // spi_inst_t *hw_inst,
        12,    // uint miso_gpio,
        15,    // uint mosi_gpio,
        14     // uint sck_gpio
    );
    spi_t *spi_p = FatFs::add_spi_p(spi);

    /* Hardware Configuration of the SD Card objects */

    FatFs_SdCardSpi spi_sd_card(
        spi_p,  // spi_t *spi_p,
        "0:",   // const char *pcName,
        9,      // uint ss_gpio,  // Slave select for this SD card
        true,   // bool use_card_detect = false,
        13,     // uint card_detect_gpio = 0,       // Card detect; ignored if !use_card_detect
        1       // uint card_detected_true = false  // Varies with card socket; ignored if !use_card_detect
    );
    FatFs::add_sd_card_p(spi_sd_card);

    static char name1[] = "1:";

    // Add another SD card:
    FatFs_SdCardSdio sdio_sd_card(
        name1,  // const char *pcName,
        18,     // uint CMD_gpio,
        19,     // uint D0_gpio,  // D0
        true,   // bool use_card_detect = false,
        16,     // uint card_detect_gpio = 0,    // Card detect, ignored if !use_card_detect
        1       // uint card_detected_true = 0,  // Varies with card socket, ignored if !use_card_detect
    );
    FatFs::add_sd_card_p(sdio_sd_card);
}
void loop() {
    for (size_t i = 0; i < FatFs::SdCard_get_num(); ++i)
        test(FatFs::SdCard_get_by_num(i));
}