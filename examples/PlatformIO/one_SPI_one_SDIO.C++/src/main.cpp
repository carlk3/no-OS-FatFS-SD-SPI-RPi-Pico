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

#include <string.h>

#include "FatFsSd.h"
#include "SerialUART.h"
#include "iostream/ArduinoStream.h"

// Serial output stream
ArduinoOutStream cout(Serial1);

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
        ;                     // Serial is via USB; wait for enumeration
    cout << "\033[2J\033[H";  // Clear Screen
    cout << "Hello, world!" << endl;

    /* Hardware Configuration of SPI object */

    // GPIO numbers, not Pico pin numbers!
    FatFsNs::SpiCfg spi(
        spi1,             // spi_inst_t *hw_inst,
        12,               // uint miso_gpio,
        15,               // uint mosi_gpio,
        14,               // uint sck_gpio
        25 * 1000 * 1000  // uint baud_rate
    );
    FatFsNs::spi_handle_t spi_handle(FatFsNs::FatFs::add_spi(spi));

    /* Hardware Configuration of the SD Card objects */

    FatFsNs::SdCardSpiCfg spi_sd_card(
        spi_handle,  // spi_handle_t spi_handle,
        "0:",        // const char *pcName,
        9,           // uint ss_gpio,  // Slave select for this SD card
        true,        // bool use_card_detect = false,
        13,          // uint card_detect_gpio = 0,       // Card detect; ignored if !use_card_detect
        1            // uint card_detected_true = false  // Varies with card socket; ignored if !use_card_detect
    );
    FatFsNs::FatFs::add_sd_card(spi_sd_card);

    // Add another SD card:
    FatFsNs::SdCardSdioCfg sdio_sd_card(
        "1:",      // const char *pcName,
        18,        // uint CMD_gpio,
        19,        // uint D0_gpio,  // D0
        true,      // bool use_card_detect = false,
        16,        // uint card_detect_gpio = 0,    // Card detect, ignored if !use_card_detect
        1,         // uint card_detected_true = 0,  // Varies with card socket, ignored if !use_card_detect
        pio1,      // PIO SDIO_PIO = pio0,          // either pio0 or pio1
        DMA_IRQ_1  // uint DMA_IRQ_num = DMA_IRQ_0  // DMA_IRQ_0 or DMA_IRQ_1
    );
    FatFsNs::FatFs::add_sd_card(sdio_sd_card);
}

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define CHK_FRESULT(s, fr)                                       \
    if (FR_OK != fr) {                                           \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

void ls(const char *dir) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *dir_str;
    if (dir[0]) {
        dir_str = dir;
    } else {
        fr = FatFsNs::Dir::getcwd(cwdbuf, sizeof cwdbuf);
        CHK_FRESULT("getcwd", fr);
        dir_str = cwdbuf;
    }
    cout << "Directory Listing: " << dir_str << endl;
    FILINFO fno = {}; /* File information */
    FatFsNs::Dir dirobj;
    fr = dirobj.findfirst(&fno, dir_str, "*");
    CHK_FRESULT("findfirst", fr);
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        cout << fno.fname << " [" << pcAttrib << "]"
             << "[size=" << fno.fsize << "]" << endl;

        fr = dirobj.findnext(&fno); /* Search for next item */
    }
    dirobj.closedir();
}

static void test(FatFsNs::SdCard *SdCard_p) {
    FRESULT fr;

    cout << endl << "Testing drive " << SdCard_p->get_name() << endl;

    fr = SdCard_p->mount();
    CHK_FRESULT("mount", fr);
    fr = FatFsNs::FatFs::chdrive(SdCard_p->get_name());
    CHK_FRESULT("chdrive", fr);
    ls(NULL);

    FatFsNs::File file;
    fr = file.open("filename.txt", FA_OPEN_APPEND | FA_WRITE);
    CHK_FRESULT("open", fr);
    {
        char const *const str = "Hello, world!\n";
        if (file.printf(str) < strlen(str)) {
            cout << "printf failed" << endl;
            ;
            for (;;) __breakpoint();
        }
    }
    fr = file.close();
    CHK_FRESULT("close", fr);

    ls("/");

    fr = FatFsNs::Dir::mkdir("subdir");
    if (FR_OK != fr && FR_EXIST != fr) {
        cout << "mkdir error: " << FRESULT_str(fr) << "(" << fr << ")" << endl;
        for (;;) __breakpoint();
    }
    fr = FatFsNs::Dir::chdir("subdir");
    CHK_FRESULT("chdir", fr);
    fr = file.open("filename2.txt", FA_OPEN_APPEND | FA_WRITE);
    CHK_FRESULT("open", fr);
    {
        char const *const str = "Hello again\n";
        UINT bw;
        fr = file.write(str, strlen(str) + 1, &bw);
        CHK_FRESULT("write", fr);
        if (strlen(str) + 1 != bw) {
            cout << "Short write!" << endl;
            ;
            for (;;) __breakpoint();
        }
    }
    fr = file.close();
    CHK_FRESULT("close", fr);

    ls(NULL);

    fr = FatFsNs::Dir::chdir("/");
    CHK_FRESULT("chdir", fr);

    ls(NULL);

    fr = SdCard_p->unmount();
    CHK_FRESULT("unmount", fr);
}

void loop() {
    for (size_t i = 0; i < FatFsNs::FatFs::SdCard_get_num(); ++i)
        test(FatFsNs::FatFs::SdCard_get_by_num(i));
    sleep_ms(1000);
}