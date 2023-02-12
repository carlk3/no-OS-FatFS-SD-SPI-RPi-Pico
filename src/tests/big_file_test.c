/* big_file_test.c
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

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "pico/stdlib.h"
//
#include "ff.h"
#include "f_util.h"

#define FF_MAX_SS 512
#define BUFFSZ (32 * FF_MAX_SS)  // Should be a factor of 1 Mebibyte

#define PRE_ALLOCATE false

typedef uint32_t DWORD;
typedef unsigned int UINT;

// Create a file of size "size" bytes filled with random data seeded with "seed"
static bool create_big_file(const char *const pathname, uint64_t size,
                            unsigned seed, DWORD *buff) {
    FRESULT fr;
    FIL file; /* File object */

    srand(seed);  // Seed pseudo-random number generator

    printf("Writing...\n");
    absolute_time_t xStart = get_absolute_time();

    /* Open the file, creating the file if it does not already exist. */
    FILINFO fno;
    size_t fsz = 0;
    fr = f_stat(pathname, &fno);
    if (FR_OK == fr)
        fsz = fno.fsize;
    if (0 < fsz && fsz <= size) {
        // This is an attempt at optimization:
        // rewriting the file should be faster than
        // writing it from scratch.
        fr = f_open(&file, pathname, FA_READ | FA_WRITE);
        if (FR_OK != fr) {
            printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            printf("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
    } else {
        fr = f_open(&file, pathname, FA_WRITE | FA_CREATE_ALWAYS);
        if (FR_OK != fr) {
            printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
    }
    if (PRE_ALLOCATE) {
        FRESULT fr = f_lseek(&file, size);
        if (FR_OK != fr) {
            printf("f_lseek error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
        if (f_tell(&file) != size) {
            printf("Disk full?\n");
            return false;
        }
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            printf("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
    }
    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        size_t n;
        for (n = 0; n < BUFFSZ / sizeof(DWORD); n++) buff[n] = rand();
        UINT bw;
        fr = f_write(&file, buff, BUFFSZ, &bw);
        if (bw < BUFFSZ) {
            printf("f_write(%s,,%d,): only wrote %d bytes\n", pathname, BUFFSZ, bw);
            return false;
        }
        if (FR_OK != fr) {
            printf("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
    }
    /* Close the file */
    f_close(&file);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
    printf("Transfer rate %.3g KiB/s (%.3g kB/s) (%.3g kb/s)\n",
           (double)size / elapsed / 1024, (double)size / elapsed / 1000, 8.0 * size / elapsed / 1000);
    return true;
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
static bool check_big_file(char *pathname, uint64_t size,
                           uint32_t seed, DWORD *buff) {
    FRESULT fr;
    FIL file; /* File object */

    srand(seed);  // Seed pseudo-random number generator

    fr = f_open(&file, pathname, FA_READ);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    printf("Reading...\n");
    absolute_time_t xStart = get_absolute_time();

    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        UINT br;
        fr = f_read(&file, buff, BUFFSZ, &br);
        if (br < BUFFSZ) {
            printf("f_read(,%s,%d,):only read %u bytes\n", pathname, BUFFSZ, br);
            return false;
        }
        if (FR_OK != fr) {
            printf("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
            return false;
        }
        /* Check the buffer is filled with the expected data. */
        size_t n;
        for (n = 0; n < BUFFSZ / sizeof(DWORD); n++) {
            unsigned int expected = rand();
            unsigned int val = buff[n];
            if (val != expected) {
                printf("Data mismatch at dword %llu: expected=0x%8x val=0x%8x\n",
                       (i * sizeof(buff)) + n, expected, val);
                return false;
            }
        }
    }
    /* Close the file */
    f_close(&file);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
    printf("Transfer rate %.3g KiB/s (%.3g kB/s) (%.3g kb/s)\n",
           (double)size / elapsed / 1024, (double)size / elapsed / 1000, 8.0 * size / elapsed / 1000);
    return true;
}
// Specify size in Mebibytes (1024x1024 bytes)
void big_file_test(char * pathname, size_t size_MiB, uint32_t seed) {
    //  /* Working buffer */
    DWORD *buff = malloc(BUFFSZ);
    assert(buff);
    assert(size_MiB);
    if (4095 < size_MiB) {
        printf("Warning: Maximum file size: 2^32 - 1 bytes on FAT volume\n");
    }
    uint64_t size_B = (uint64_t)size_MiB * 1024 * 1024;

    if (create_big_file(pathname, size_B, seed, buff))
        check_big_file(pathname, size_B, seed, buff);

    free(buff);
}

/* [] END OF FILE */
