/* Instead of a statically linked hw_config.c,
   create configuration dynamically */

#include <stdio.h>
#include <stdlib.h>
//
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
//
#include "hw_config.h"
//
#include "diskio.h" /* Declarations of disk functions */

void add_spi(spi_t *const spi);
void add_sd_card(sd_card_t *const sd_card);

static spi_t *p_spi;
void spi0_dma_isr() { spi_irq_handler(p_spi); }

int main() {
    stdio_init_all();
    time_init();

    puts("Hello, world!");

    // Hardware Configuration of SPI "object"
    p_spi = (spi_t *)malloc(sizeof(spi_t));
    if (!p_spi) panic("Out of memory");
    p_spi->hw_inst = spi0;  // SPI component
    p_spi->miso_gpio = 16;  // GPIO number (not pin number)
    p_spi->mosi_gpio = 19;
    p_spi->sck_gpio = 18;
    p_spi->baud_rate = 12500 * 1000;  // The limitation here is SPI slew rate.
    p_spi->dma_isr = spi0_dma_isr;
    p_spi->initialized = false;  // initialized flag

    // Hardware Configuration of the SD Card "object"
    sd_card_t *p_sd_card = (sd_card_t *)malloc(sizeof(sd_card_t));
    if (!p_sd_card) panic("Out of memory");
    p_sd_card->pcName = "0:";  // Name used to mount device
    p_sd_card->spi = p_spi;    // Pointer to the SPI driving this card
    p_sd_card->ss_gpio = 17;   // The SPI slave select GPIO for this SD card
    p_sd_card->card_detect_gpio = 22;  // Card detect

    // What the GPIO read returns when a card is
    // present. Use -1 if there is no card detect.
    p_sd_card->card_detected_true = 1;

    // State attributes:
    p_sd_card->m_Status = STA_NOINIT;
    p_sd_card->sectors = 0;
    p_sd_card->card_type = 0;

    add_spi(p_spi);
    add_sd_card(p_sd_card);

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char *const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

    puts("Goodbye, world!");
    for (;;)
        ;
}
