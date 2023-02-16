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
//
#include "SerialUART.h"
#include "iostream/ArduinoStream.h"

using namespace FatFsNs;

// Serial output stream
ArduinoOutStream cout(Serial1);

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define CHK_RESULT(s, fr)                                        \
    if (FR_OK != fr) {                                           \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

void setup() {
    Serial1.begin(115200);  // set up Serial library
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration

    /* This example assumes the following wiring:
        | signal | SPI1 | GPIO | card | Description            |
        | ------ | ---- | ---- | ---- | ---------------------- |
        | MISO   | RX   | 12   |  DO  | Master In, Slave Out   |
        | CS0    | CSn  | 09   |  CS  | Slave (or Chip) Select |
        | SCK    | SCK  | 14   | SCLK | SPI clock              |
        | MOSI   | TX   | 15   |  DI  | Master Out, Slave In   |
        | CD     |      | 13   |  DET | Card Detect            |
    */

    // GPIO numbers, not Pico pin numbers!

    /*
    Hardware Configuration of SPI "objects"
    Note: multiple SD cards can be driven by one SPI if they use different slave selects.
    Note: None, either or both of the RP2040 SPI components can be used.
    */

    // Hardware Configuration of SPI object

    SpiCfg spi(
        spi1,             // spi_inst_t *hw_inst,
        12,               // uint miso_gpio,
        15,               // uint mosi_gpio,
        14,               // uint sck_gpio
        25 * 1000 * 1000  // uint baud_rate
    );
    spi_handle_t spi_handle(FatFs::add_spi(spi));

    // Hardware Configuration of the SD Card object

    SdCardSpiCfg spi_sd_card(
        spi_handle,  // spi_handle_t spi_handle,
        "0:",        // const char *pcName,
        9,           // uint ss_gpio,  // Slave select for this SD card
        true,        // bool use_card_detect = false,
        13,          // uint card_detect_gpio = 0,       // Card detect; ignored if !use_card_detect
        1            // uint card_detected_true = false  // Varies with card socket; ignored if !use_card_detect
    );

    FatFsNs::SdCard* card_p(FatFs::add_sd_card(spi_sd_card));

    /* ********************************************************************** */
    cout << "\033[2J\033[H";  // Clear Screen
    cout << "Hello, world!" << endl;
    // sd_card_t* pSD = sd_get_by_num(0);
    FRESULT fr = card_p->mount();
    CHK_RESULT("mount", fr);
    File file;
    char const* const filename = "filename.txt";
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    CHK_RESULT("open", fr);
    char const* const str = "Hello, world!\n";
    if (file.printf(str) < strlen(str)) {
        cout << "printf failed\n"
             << endl;
        for (;;) __breakpoint();
    }
    fr = file.close();
    CHK_RESULT("close", fr);
    fr = card_p->unmount();
    CHK_RESULT("unmount", fr);

    cout << "Goodbye, world!" << endl;
}
void loop() {}