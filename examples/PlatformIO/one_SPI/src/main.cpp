
#include "FatFsSd.h"
//
#include "SerialUART.h"

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
    if (num <= sd_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

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