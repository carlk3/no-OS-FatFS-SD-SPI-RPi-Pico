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

#include "FatFsSd.h"
//
#include "SerialUART.h"

/* Infrastructure*/
extern "C" int printf(const char *__restrict format, ...) {
    char buf[256] = {0};
    va_list xArgs;
    va_start(xArgs, format);
    vsnprintf(buf, sizeof buf, format, xArgs);
    va_end(xArgs);
    return Serial1.printf("%s", buf);
}
extern "C" int puts(const char *s) {
    return Serial1.println(s);
}

/* ********************************************************************** */

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device            
        .type = SD_IF_SDIO,
        /* 
        Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
        The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
            CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
            As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30, 
              which is -2 in mod32 arithmetic, so:
            CLK_gpio = D0_gpio -2.
            D1_gpio = D0_gpio + 1;
            D2_gpio = D0_gpio + 2;
            D3_gpio = D0_gpio + 3;
        */
        .sdio_if = {
            .CMD_gpio = 18,
            .D0_gpio = 19,
            .SDIO_PIO = pio1,
            .DMA_IRQ_num = DMA_IRQ_1
        },
        .use_card_detect = true,    
        .card_detect_gpio = 16,   // Card detect
        .card_detected_true = 1   // What the GPIO read returns when a card is
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
// These need to be defined for the API:
extern "C" size_t spi_get_num() { return 0; }
extern "C" spi_t *spi_get_by_num(size_t num) { return NULL; }

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
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "filename.txt";
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
}
void loop() {}