#include <stdio.h>
#include "pico/stdlib.h"

#include "ff.h"
#include "f_util.h"
#include "rtc.h"

int main() {
    stdio_init_all();
    time_init();

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface", http://elm-chan.org/fsw/ff/00index_e.html
    static FATFS fatfs;
    FRESULT fr = f_mount(&fatfs, "", 1); 
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char * const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount("");

    puts("Goodbye, world!");
    for (;;);
}
