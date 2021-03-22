// Adapted from "FATFileSystem example"
//  at https://os.mbed.com/docs/mbed-os/v5.15/apis/fatfilesystem.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "ff.h"
#include "f_util.h"

// Maximum number of elements in buffer
#define BUFFER_MAX_LEN 10

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf

void ls() {
    char pcWriteBuffer[128] = {0};

    FRESULT fr; /* Return value */
    fr = f_getcwd(pcWriteBuffer, sizeof pcWriteBuffer);
    if (FR_OK != fr) {
        printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    printf("Directory Listing: %s\n", pcWriteBuffer);

    DIR dj;      /* Directory object */
    FILINFO fno; /* File information */
    memset (&dj, 0, sizeof dj);
    memset (&fno, 0, sizeof fno);
    TRACE_PRINTF("%s: f_findfirst(path=%s)\n", __func__, pcWriteBuffer);
    fr = f_findfirst(&dj, &fno, pcWriteBuffer, "*");
    if (FR_OK != fr) {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
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
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno); /* Search for next item */
    }

    f_closedir(&dj);
}

void simple() {
    printf("\nSimple Test\n");

    // Open the numbers file
    printf("Opening \"numbers.txt\"... ");
    FIL f;
    FRESULT fc = f_open(&f, "numbers.txt", FA_READ | FA_WRITE);
    printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
    fflush(stdout);
    if (FR_OK != fc && FR_NO_FILE != fc) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fc), fc);
        return;
    } else if (FR_NO_FILE == fc) {
        // Create the numbers file if it doesn't exist
        printf("No file found, creating a new file... ");
        fflush(stdout);
        fc = f_open(&f, "numbers.txt",
                    FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
        printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
        if (FR_OK != fc) printf("f_open error: %s (%d)\n", FRESULT_str(fc), fc);
        fflush(stdout);
        for (int i = 0; i < 10; i++) {
            printf("\rWriting numbers (%d/%d)... ", i, 10);
            fflush(stdout);
            fc = f_printf(&f, "    %d\n", i);
            if (FR_OK != fc) {
                printf("Fail :(\n");
                printf("f_printf error: %s (%d)\n", FRESULT_str(fc), fc);
            }
        }
        printf("\rWriting numbers (%d/%d)... OK\n", 10, 10);
        fflush(stdout);

        printf("Seeking file... ");
        fc = f_lseek(&f, 0);
        printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
        if (FR_OK != fc) printf("f_lseek error: %s (%d)\n", FRESULT_str(fc), fc);
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
    fc = f_close(&f);
    printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
    if (FR_OK != fc) printf("f_close error: %s (%d)\n", FRESULT_str(fc), fc);
    fflush(stdout);

    ls();

    fc = f_chdir("/");
    if (FR_OK != fc) printf("chdir error: %s (%d)\n", FRESULT_str(fc), fc);

    ls();

    // Display the numbers file
    printf("Opening \"numbers.txt\"... ");
    fc = f_open(&f, "numbers.txt", FA_READ);
    printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
    if (FR_OK != fc) printf("f_open error: %s (%d)\n", FRESULT_str(fc), fc);
    fflush(stdout);

    printf("numbers:\n");
    while (!f_eof(&f)) {
        // int c = f_getc(f);
        char c;
        UINT br;
        fc = f_read(&f, &c, sizeof c, &br);
        if (FR_OK != fc)
            printf("f_read error: %s (%d)\n", FRESULT_str(fc), fc);
        else
            printf("%c", c);
    }

    printf("\nClosing \"numbers.txt\"... ");
    fc = f_close(&f);
    printf("%s\n", (FR_OK != fc ? "Fail :(" : "OK"));
    if (FR_OK != fc) printf("f_close error: %s (%d)\n", FRESULT_str(fc), fc);
    fflush(stdout);
}
