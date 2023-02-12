/* simple.c
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
// Adapted from "FATFileSystem example"
//  at https://os.mbed.com/docs/mbed-os/v5.15/apis/fatfilesystem.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "f_util.h"
#include "ff.h"

// Maximum number of elements in buffer
#define BUFFER_MAX_LEN 10

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf

extern void ls(const char *dir);

void simple() {
    printf("\nSimple Test\n");

    char cwdbuf[FF_LFN_BUF - 12] = {0};
    FRESULT fr = f_getcwd(cwdbuf, sizeof cwdbuf);
    if (FR_OK != fr) {
        printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    // Open the numbers file
    printf("Opening \"numbers.txt\"... ");
    FIL f;
    fr = f_open(&f, "numbers.txt", FA_READ | FA_WRITE);
    printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
    fflush(stdout);
    if (FR_OK != fr && FR_NO_FILE != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    } else if (FR_NO_FILE == fr) {
        // Create the numbers file if it doesn't exist
        printf("No file found, creating a new file... ");
        fflush(stdout);
        fr = f_open(&f, "numbers.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
        printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
        if (FR_OK != fr) printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        fflush(stdout);
        for (int i = 0; i < 10; i++) {
            printf("\rWriting numbers (%d/%d)... ", i, 10);
            fflush(stdout);
            // When the string was written successfuly, it returns number of
            // character encoding units written to the file. When the function
            // failed due to disk full or any error, a negative value will be
            // returned.
            int rc = f_printf(&f, "    %d\n", i);
            if (rc < 0) {
                printf("Fail :(\n");
                printf("f_printf error: %s (%d)\n", FRESULT_str(fr), fr);
            }
        }
        printf("\rWriting numbers (%d/%d)... OK\n", 10, 10);
        fflush(stdout);

        printf("Seeking file... ");
        fr = f_lseek(&f, 0);
        printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
        if (FR_OK != fr)
            printf("f_lseek error: %s (%d)\n", FRESULT_str(fr), fr);
        fflush(stdout);
    }
    // Go through and increment the numbers
    for (int i = 0; i < 10; i++) {
        printf("\nIncrementing numbers (%d/%d)... ", i, 10);

        // Get current stream position
        long pos = f_tell(&f);

        // Parse out the number and increment
        char buf[BUFFER_MAX_LEN];
        if (!f_gets(buf, BUFFER_MAX_LEN, &f)) {
            printf("error: f_gets returned NULL\n");
        }
        char *endptr;
        int32_t number = strtol(buf, &endptr, 10);
        if ((endptr == buf) ||            // No character was read
            (*endptr && *endptr != '\n')  // The whole input was not converted
        ) {
            continue;
        }
        number += 1;

        // Seek to beginning of number
        f_lseek(&f, pos);

        // Store number
        f_printf(&f, "    %d\n", (int)number);

        // Flush between write and read on same file
        f_sync(&f);
    }
    printf("\rIncrementing numbers (%d/%d)... OK\n", 10, 10);
    fflush(stdout);

    // Close the file which also flushes any cached writes
    printf("Closing \"numbers.txt\"... ");
    fr = f_close(&f);
    printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    fflush(stdout);

    ls("");

    fr = f_chdir("/");
    if (FR_OK != fr) printf("chdir error: %s (%d)\n", FRESULT_str(fr), fr);

    ls("");

    // Display the numbers file
    char pathbuf[FF_LFN_BUF] = {0};
    snprintf(pathbuf, sizeof pathbuf, "%s/%s", cwdbuf, "numbers.txt");
    printf("Opening \"%s\"... ", pathbuf);
    fr = f_open(&f, pathbuf, FA_READ);
    printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
    if (FR_OK != fr) printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
    fflush(stdout);

    printf("numbers:\n");
    while (!f_eof(&f)) {
        // int c = f_getc(f);
        char c;
        UINT br;
        fr = f_read(&f, &c, sizeof c, &br);
        if (FR_OK != fr)
            printf("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
        else
            printf("%c", c);
    }

    printf("\nClosing \"%s\"... ", pathbuf);
    fr = f_close(&f);
    printf("%s\n", (FR_OK != fr ? "Fail :(" : "OK"));
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    fflush(stdout);
}
