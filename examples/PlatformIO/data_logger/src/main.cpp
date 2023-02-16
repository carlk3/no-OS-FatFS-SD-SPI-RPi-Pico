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

/*
This example reads analog input A0 and logs the voltage
  in a file on SD cards once per second.

It also demonstates a way to do static configuration.
*/
#include <time.h>

#include "FatFsSd.h"
#include "SerialUART.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "iostream/ArduinoStream.h"
#include "pico/stdlib.h"

// Serial output stream
ArduinoOutStream cout(Serial1);

using namespace FatFsNs;

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define FAIL(s, fr)                                              \
    {                                                            \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

#define CHK_FRESULT(s, fr) \
    {                      \
        if (FR_OK != fr)   \
            FAIL(s, fr);   \
    }

#define ASSERT(pred)                                       \
    {                                                      \
        if (!(pred)) {                                     \
            cout << __FILE__ << ":" << __LINE__ << ": "    \
                 << "Assertion failed: " << #pred << endl; \
            for (;;) __breakpoint();                       \
        }                                                  \
    }

/* ********************************************************************** */

static bool print_heading(File& file) {
    FRESULT fr = file.lseek(file.size());
    CHK_FRESULT("lseek", fr);
    if (0 == file.tell()) {
        // Print header
        if (file.printf("Date,Time,Voltage\n") < 0) {
           cout << "printf error" << endl;
            return false;
        }
    }
    return true;
}

static bool open_file(File& file) {
    const time_t timer = time(NULL);
    struct tm tmbuf;
    localtime_r(&timer, &tmbuf);
    char filename[64];
    int n = snprintf(filename, sizeof filename, "/data");
    ASSERT(0 < n && n < (int)sizeof filename);
    FRESULT fr = Dir::mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("mkdir", fr);
        return false;
    }
    //  tm_year	int	years since 1900
    //  tm_mon	int	months since January	0-11
    //  tm_mday	int	day of the month	1-31
    n += snprintf(filename + n, sizeof filename - n, "/%04d-%02d-%02d",
                  tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday);
    ASSERT(0 < n && n < (int)sizeof filename);
    fr = Dir::mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("mkdir", fr);
        return false;
    }
    size_t nw = strftime(filename + n, sizeof filename - n, "/%H.csv", &tmbuf);
    ASSERT(nw);
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("open", fr);
        return false;
    }
    if (!print_heading(file)) return false;
    return true;
}

bool process_logger() {
    /* It's very inefficient to open and close the file for every record,
    but you're less likely to lose data that way.  But also see f_sync
    (http://elm-chan.org/fsw/ff/doc/sync.html). */

    FRESULT fr;
    File file;

    bool rc = open_file(file);
    if (!rc) return false;

    // Form date-time string
    char buf[128];
    const time_t secs = time(NULL);
    struct tm tmbuf;
    struct tm* ptm = localtime_r(&secs, &tmbuf);
    size_t n = strftime(buf, sizeof buf, "%F,%T,", ptm);
    ASSERT(n);

    /* Assuming something analog is connected to A0 */
    int sensorValue = analogRead(A0);
    float voltage = 3.3f * sensorValue / 1024;
    int nw = snprintf(buf + n, sizeof buf - n, "%.3f\n", (double)voltage);
    // Notice that only when this returned value is non-negative and less than n,
    //   the string has been completely written.
    ASSERT(0 < nw && nw < (int)sizeof buf);
    n += nw;
    cout << buf;

    UINT bw;
    fr = file.write(buf, n, &bw);
    CHK_FRESULT("write", fr);
    if (bw < n) {
        cout << "Short write!" << endl;
        for (;;) __breakpoint();
    }
    fr = file.close();
    CHK_FRESULT("close", fr);
    return true;
}

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    cout << "Hello, world!" << endl;

    time_init();
    // You might want to ask the user for the time,
    //   but it is hardcoded here for simplicity:
    datetime_t t = {
        .year = 2023,
        .month = 2,
        .day = 10,
        .dotw = 5,  // 0 is Sunday, so 5 is Friday
        .hour = 17,
        .min = 5,
        .sec = 0};
    rtc_set_datetime(&t);

/*
This example assumes the following wiring for the SD card:

    | signal | SPI1 | GPIO | card | Description            |
    | ------ | ---- | ---- | ---- | ---------------------- |
    | MISO   | RX   | 12   |  DO  | Master In, Slave Out   |
    | CS0    | CSn  | 09   |  CS  | Slave (or Chip) Select |
    | SCK    | SCK  | 14   | SCLK | SPI clock              |
    | MOSI   | TX   | 15   |  DI  | Master Out, Slave In   |
    | CD     |      | 13   |  DET | Card Detect            |
*/

    /* Hardware Configuration of SPI object */

    // GPIO numbers, not Pico pin numbers!
    SpiCfg spi(
        spi1,             // spi_inst_t *hw_inst,
        12,               // uint miso_gpio,
        15,               // uint mosi_gpio,
        14,               // uint sck_gpio
        25 * 1000 * 1000  // uint baud_rate
    );
    spi_handle_t spi_handle = FatFs::add_spi(spi);

    /* Hardware Configuration of the SD Card object */

    SdCardSpiCfg spi_sd_card(
        spi_handle,  // spi_handle_t spi_handle,
        "0:",        // const char *pcName,
        9,           // uint ss_gpio,  // Slave select for this SD card
        true,        // bool use_card_detect = false,
        13,          // uint card_detect_gpio = 0,       // Card detect; ignored if !use_card_detect
        1            // uint card_detected_true = false  // Varies with card socket; ignored if !use_card_detect
    );
    FatFsNs::SdCard* SdCard_p(FatFs::add_sd_card(spi_sd_card));

    FRESULT fr = SdCard_p->mount();
    CHK_FRESULT("mount", fr);

    next_log_time = delayed_by_ms(get_absolute_time(), period);
    logger_enabled = true;
}

void loop() {
    if (absolute_time_diff_us(get_absolute_time(), next_log_time) < 0) {
        FRESULT fr;
        if (logger_enabled) {
            if (!process_logger()) 
                logger_enabled= false;
        }
        next_log_time = delayed_by_ms(next_log_time, period);
    }
}