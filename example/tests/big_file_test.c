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
//#include "ff_headers.h"
#include "ff_stdio.h"

#define FF_MAX_SS 512
#define BUFFSZ 8 * 1024

#undef assert
#define assert configASSERT
#define fopen ff_fopen
#define fwrite ff_fwrite
#define fread ff_fread
#define fclose ff_fclose
#ifndef FF_DEFINED
#define errno stdioGET_ERRNO()
#define free vPortFree
#define malloc pvPortMalloc
#endif

typedef uint32_t DWORD;
typedef unsigned int UINT;

// Borrowed from http://elm-chan.org/fsw/ff/res/app4.c
static DWORD pn(/* Pseudo random number generator */
                DWORD pns /* 0:Initialize, !0:Read */  // Is that right? -- CK3
) {
    static DWORD lfsr;
    UINT n;

    if (pns) {
        lfsr = pns;
        for (n = 0; n < 32; n++) pn(0);
    }
    if (lfsr & 1) {
        lfsr >>= 1;
        lfsr ^= 0x80200003;
    } else {
        lfsr >>= 1;
    }
    return lfsr;
}

// Create a file of size "size" bytes filled with random data seeded with "seed"
static bool create_big_file(const char *const pathname, size_t size,
                            unsigned seed) {
    int32_t lItems;
    FF_FILE *pxFile;

    //    DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */
    size_t bufsz = size < BUFFSZ ? size : BUFFSZ;
    assert(0 == size % bufsz);
    DWORD *buff = malloc(bufsz);
    assert(buff);

    pn(seed);  // See pseudo-random number generator

    printf("Writing...\n");
    absolute_time_t xStart = get_absolute_time();

    /* Open the file, creating the file if it does not already exist. */
    FF_Stat_t xStat;
    size_t fsz = 0;
    if (ff_stat(pathname, &xStat) == 0) fsz = xStat.st_size;
    if (0 < fsz && fsz <= size) {
        // This is an attempt at optimization:
        // rewriting the file should be faster than
        // writing it from scratch.
        pxFile = ff_fopen(pathname, "r+");
        ff_rewind(pxFile);
    } else {
        pxFile = ff_fopen(pathname, "w");
    }
    //    // FF_FILE *ff_truncate( const char * pcFileName, long lTruncateSize
    //    );
    //    //   If the file was shorter than lTruncateSize
    //    //   then new data added to the end of the file is set to 0.
    //    pxFile = ff_truncate(pathname, size);
    if (!pxFile) {
        printf("ff_fopen(%s): %s (%d)\n", pathname, strerror(errno), errno);
        return false;
    }
    assert(pxFile);

    size_t i;
    for (i = 0; i < size / bufsz; ++i) {
        size_t n;
        for (n = 0; n < bufsz / sizeof(DWORD); n++) buff[n] = pn(0);
        lItems = fwrite(buff, bufsz, 1, pxFile);
        if (lItems < 1)
            printf("fwrite(%s): %s (%d)\n", pathname, strerror(errno), errno);
        assert(lItems == 1);
    }
    free(buff);
    /* Close the file. */
    ff_fclose(pxFile);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
    printf("Transfer rate %.3g KiB/s\n", (double)size / elapsed / 1024);
    return true;
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
static void check_big_file(const char *const pathname, size_t size,
                           uint32_t seed) {
    int32_t lItems;
    FF_FILE *pxFile;

    //    DWORD buff[FF_MAX_SS];  /* Working buffer (4 sector in size) */
    //	assert(0 == size % sizeof(buff));
    size_t bufsz = size < BUFFSZ ? size : BUFFSZ;
    assert(0 == size % bufsz);
    DWORD *buff = malloc(bufsz);
    assert(buff);

    pn(seed);

    /* Open the file, creating the file if it does not already exist. */
    pxFile = fopen(pathname, "r");
    if (!pxFile)
        printf("fopen(%s): %s (%d)\n", pathname, strerror(errno), -errno);
    assert(pxFile);

    printf("Reading...\n");
    absolute_time_t xStart = get_absolute_time();

    size_t i;
    for (i = 0; i < size / bufsz; ++i) {
        lItems = fread(buff, bufsz, 1, pxFile);
        if (lItems < 1)
            printf("fread(%s): %s (%d)\n", pathname, strerror(errno), -errno);
        assert(lItems == 1);

        /* Check the buffer is filled with the expected data. */
        size_t n;
        for (n = 0; n < bufsz / sizeof(DWORD); n++) {
            unsigned int expected = pn(0);
            unsigned int val = buff[n];
            if (val != expected)
                printf("Data mismatch at dword %u: expected=0x%8x val=0x%8x\n",
                       (i * sizeof(buff)) + n, expected, val);
        }
    }
    free(buff);
    /* Close the file. */
    ff_fclose(pxFile);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
    printf("Transfer rate %.3g KiB/s\n", (double)size / elapsed / 1024);
}

// Create a file of size "size" bytes filled with random data seeded with "seed"
// static
void create_big_file_v1(const char *const pathname, size_t size,
                        unsigned seed) {
    int32_t lItems;
    FF_FILE *pxFile;
    int val;

    assert(0 == size % sizeof(int));

    srand(seed);

    /* Open the file, creating the file if it does not already exist. */
    pxFile = ff_fopen(pathname, "w");
    if (!pxFile)
        printf("ff_fopen(%s): %s (%d)\n", pathname, strerror(errno), errno);
    assert(pxFile);

    printf("Writing...\n");
    absolute_time_t xStart = get_absolute_time();

    size_t i;
    for (i = 0; i < size / sizeof(val); ++i) {
        val = rand();
        lItems = ff_fwrite(&val, sizeof(val), 1, pxFile);
        if (lItems < 1)
            printf("ff_fwrite(%s): %s (%d)\n", pathname, strerror(errno),
                   errno);
        assert(lItems == 1);
    }
    /* Close the file. */
    ff_fclose(pxFile);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
// static
void check_big_file_v1(const char *const pathname, size_t size, uint32_t seed) {
    int32_t lItems;
    FF_FILE *pxFile;

    assert(0 == size % sizeof(int));

    srand(seed);

    /* Open the file, creating the file if it does not already exist. */
    pxFile = ff_fopen(pathname, "r");
    if (!pxFile)
        printf("ff_fopen(%s): %s (%d)\n", pathname, strerror(errno), errno);
    assert(pxFile);

    printf("Reading...\n");
    absolute_time_t xStart = get_absolute_time();

    size_t i;
    int val;
    for (i = 0; i < size / sizeof(val); ++i) {
        lItems = ff_fread(&val, sizeof(val), 1, pxFile);
        if (lItems < 1)
            printf("ff_fread(%s): %s (%d)\n", pathname, strerror(errno), errno);
        assert(lItems == 1);

        /* Check the buffer is filled with the expected data. */
        int expected = rand();
        if (val != expected)
            printf("Data mismatch at word %zu: expected=%d val=%d\n", i,
                   expected, val);
    }
    /* Close the file. */
    ff_fclose(pxFile);

    int64_t elapsed_us = absolute_time_diff_us(xStart, get_absolute_time());
    float elapsed = elapsed_us / 1E6;
    printf("Elapsed seconds %.3g\n", elapsed);
}

void big_file_test(const char *const pathname, size_t size, uint32_t seed) {
    if (create_big_file(pathname, size, seed)) 
        check_big_file(pathname, size, seed);
}

/* [] END OF FILE */
