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

#include <vector>

#include "FatFsSd_C.h"
#include "SerialUART.h"

#define printf Serial1.printf
#define puts Serial1.println

static const uint led_pin = PICO_DEFAULT_LED_PIN;

static std::vector<spi_t *> spis;
static std::vector<sd_card_t *> sd_cards;

/* ********************************************************************** */

extern "C" size_t sd_get_num() { return sd_cards.size(); }
extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return sd_cards[num];
    } else {
        return NULL;
    }
}
extern "C" size_t spi_get_num() { return spis.size(); }
extern "C" spi_t *spi_get_by_num(size_t num) {
    if (num <= spi_get_num()) {
        return spis[num];
    } else {
        return NULL;
    }
}
void add_spi(spi_t *spi) { spis.push_back(spi); }
void add_sd_card(sd_card_t *sd_card) { sd_cards.push_back(sd_card); }

void test(sd_card_t *sd_card_p) {

    printf("Testing drive %s\n", sd_card_p->pcName);

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    FRESULT fr = f_mount(&sd_card_p->fatfs, sd_card_p->pcName, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __BKPT(1);
    }
    fr = f_chdrive(sd_card_p->pcName);
    if (FR_OK != fr) {
        printf("f_chdrive error: %s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __BKPT(2);
    }

    FIL fil;
    const char *const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        for (;;) __BKPT(3);
    }
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
        for (;;) __BKPT(4);
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __BKPT(5);
    }
    f_unmount(sd_card_p->pcName);
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
*/

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    printf("\033[2J\033[H");  // Clear Screen
    printf("Hello!\n");

    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    time_init();

    puts("Hello, world!");

    // Hardware Configuration of SPI "object"
    spi_t *p_spi = new spi_t;
    memset(p_spi, 0, sizeof(spi_t));
    if (!p_spi) {
        printf("Out of memory");
        for (;;) __BKPT(3);
    }

    p_spi->hw_inst = spi1;  // SPI component
    p_spi->miso_gpio = 12;  // GPIO number (not pin number)
    p_spi->mosi_gpio = 15;
    p_spi->sck_gpio = 14;
    p_spi->baud_rate = 25 * 1000 * 1000;  // Actual frequency: 20833333.
    add_spi(p_spi);

    // Hardware Configuration of the SD Card "object"
    sd_card_t *p_sd_card = new sd_card_t;
    if (!p_sd_card) {
        printf("Out of memory");
        for (;;) __BKPT(6);
    }
    memset(p_sd_card, 0, sizeof(sd_card_t));
    p_sd_card->pcName = "0:";  // Name used to mount device
    p_sd_card->type = SD_IF_SPI,
    p_sd_card->spi_if.spi = p_spi;  // Pointer to the SPI driving this card
    p_sd_card->spi_if.ss_gpio = 9;  // The SPI slave select GPIO for this SD card
    p_sd_card->use_card_detect = true;
    p_sd_card->card_detect_gpio = 13;  // Card detect
    // What the GPIO read returns when a card is present:
    p_sd_card->card_detected_true = 1;
    add_sd_card(p_sd_card);

    /* Add another SD card */
    p_sd_card = new sd_card_t;
    if (!p_sd_card) {
        printf("Out of memory");
        for (;;) __BKPT(7);
    }
    memset(p_sd_card, 0, sizeof(sd_card_t));
    p_sd_card->pcName = "1:";  // Name used to mount device
    p_sd_card->type = SD_IF_SDIO;
    /*
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
    p_sd_card->sdio_if.CMD_gpio = 18;
    p_sd_card->sdio_if.D0_gpio = 19;
    p_sd_card->use_card_detect = true;
    p_sd_card->card_detect_gpio = 16;   // Card detect
    p_sd_card->card_detected_true = 1;  // What the GPIO read returns when a card is present.
    p_sd_card->sdio_if.SDIO_PIO = pio1;
    p_sd_card->sdio_if.DMA_IRQ_num = DMA_IRQ_1;
    add_sd_card(p_sd_card);

    for (size_t i = 0; i < sd_get_num(); ++i)
        test(sd_get_by_num(i));

    puts("Goodbye, world!");
}
void loop() {
    while (true) {
        printf("Goodbye!\n");
        gpio_put(led_pin, 1);
        sleep_ms(250);
        gpio_put(led_pin, 0);
        sleep_ms(250);
    }
}