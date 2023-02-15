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
  in a file on an SD card once per second.
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

/* ********************************************************************** */
/*
This example assumes the following wiring:

    | GPIO  | Pico Pin | microSD | Function    |
    | ----  | -------- | ------- | ----------- |
    |  16   |    21    | DET     | Card Detect |
    |  17   |    22    | CLK     | SDIO_CLK    |
    |  18   |    24    | CMD     | SDIO_CMD    |
    |  19   |    25    | DAT0    | SDIO_D0     |
    |  20   |    26    | DAT1    | SDIO_D1     |
    |  21   |    27    | DAT2    | SDIO_D2     |
    |  22   |    29    | DAT3    | SDIO_D3     |

*/

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",  // Name used to mount device
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
            .SDIO_PIO = pio1,         // Either pio0 or pio1
            .DMA_IRQ_num = DMA_IRQ_1  // Either DMA_IRQ_0 or DMA_IRQ_1
        },
        .use_card_detect = true,
        .card_detect_gpio = 16,  // Card detect
        .card_detected_true = 1  // What the GPIO read returns when a card is
                                 // present.
    }};
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

static bool print_heading(FatFs_File &file) {
    FRESULT fr = file.lseek(file.size());
    CHK_FRESULT("lseek", fr);
    if (0 == file.tell()) {
        // Print header
        if (file.printf("Date,Time,Voltage\n") < 0) {
            printf("f_printf error\n");
            return false;
        }
    }
    return true;
}

static bool open_file(FatFs_File &file) {
    const time_t timer = time(NULL);
    struct tm tmbuf;
    localtime_r(&timer, &tmbuf);
    char filename[64];
    int n = snprintf(filename, sizeof filename, "/data");
    ASSERT(0 < n && n < (int)sizeof filename);
    FRESULT fr = f_mkdir(filename);
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
    fr = f_mkdir(filename);
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
    FatFs_File file;

    bool rc = open_file(file);
    if (!rc) return false;

    // Form date-time string
    char buf[128];
    const time_t secs = time(NULL);
    struct tm tmbuf;
    struct tm *ptm = localtime_r(&secs, &tmbuf);
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

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *sd_card_p = sd_get_by_num(0);
    FRESULT fr = f_mount(&sd_card_p->fatfs, sd_card_p->pcName, 1);
    CHK_FRESULT("mount", fr);

    next_log_time = delayed_by_ms(get_absolute_time(), period);
    logger_enabled = true;
}

void loop() {
    if (logger_enabled &&
        absolute_time_diff_us(get_absolute_time(), next_log_time) < 0) {
        if (!process_logger()) logger_enabled = false;
        next_log_time = delayed_by_ms(next_log_time, period);
    }
}