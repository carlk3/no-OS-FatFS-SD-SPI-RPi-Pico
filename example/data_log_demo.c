/* data_log_demo.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
//
#include "hardware/adc.h"
#include "pico/stdlib.h"
//
#include "ff.h"
//
#include "f_util.h"
#include "my_debug.h"

#define DEVICENAME "0:"

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf

static bool print_header(FIL *fp) {
    TRACE_PRINTF("%s\n", __func__);
    myASSERT(fp);
    FRESULT fr = f_lseek(fp, f_size(fp));
    if (FR_OK != fr) {
        printf("f_lseek error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    if (0 == f_tell(fp)) {
        // Print header
        if (f_printf(fp, "Date,Time,Temperature (°C)\n") < 0) {
            printf("f_printf error\n");
            return false;
        }
    }
    return true;
}

static bool open_file(FIL *fp) {
    TRACE_PRINTF("%s\n", __func__);
    myASSERT(fp);
    const time_t timer = time(NULL);
    struct tm tmbuf;
    localtime_r(&timer, &tmbuf);
    char filename[64];
    int n = snprintf(filename, sizeof filename, "/data");
    myASSERT(0 < n && n < (int)sizeof filename);
    FRESULT fr = f_mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_mkdir error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    //  tm_year	int	years since 1900
    //  tm_mon	int	months since January	0-11
    //  tm_mday	int	day of the month	1-31
    n += snprintf(filename + n, sizeof filename - n, "/%04d-%02d-%02d",
                  tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday);
    myASSERT(0 < n && n < (int)sizeof filename);
    fr = f_mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_mkdir error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    size_t nw = strftime(filename + n, sizeof filename - n, "/%H.csv", &tmbuf);
    myASSERT(nw);
    fr = f_open(fp, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return false;
    }
    if (!print_header(fp)) return false;
    return true;
}

bool process_logger() {
    TRACE_PRINTF("%s\n", __func__);
    /* It's very inefficient to open and close the file for every record,
    but you're less likely to lose data that way.  But also see f_sync
    (http://elm-chan.org/fsw/ff/doc/sync.html). */
    FIL fil;
    bool rc = open_file(&fil);
    if (!rc) return false;

    // Form date-time string
    char buf[128];
    const time_t secs = time(NULL);
    struct tm tmbuf;
    struct tm *ptm = localtime_r(&secs, &tmbuf);
    size_t n = strftime(buf, sizeof buf, "%F,%T,", ptm);
    myASSERT(n);

    // The temperature sensor is on input 4:
    adc_select_input(4);
    uint16_t result = adc_read();
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = conversion_factor * result;
    TRACE_PRINTF("Raw value: 0x%03x, voltage: %f V\n", result, (double)voltage);

    // Temperature sensor values can be approximated in centigrade as:
    //    T = 27 - (ADC_Voltage - 0.706)/0.001721
    float Tc = 27.0f - (voltage - 0.706f) / 0.001721f;
    TRACE_PRINTF("Temperature: %.1f °C\n", (double)Tc);
    int nw = snprintf(buf + n, sizeof buf - n, "%.3g\n", (double)Tc);
    myASSERT(0 < nw && nw < (int)sizeof buf);

    if (f_printf(&fil, "%s", buf) < 0) {
        printf("f_printf failed\n");
        return false;
    }
    FRESULT fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    return true;
}
