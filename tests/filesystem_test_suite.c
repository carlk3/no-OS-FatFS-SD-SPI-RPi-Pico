/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
 */
/* mbed Microcontroller Library
 * Copyright (c) 2006-2019 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* FreeRTOS+FAT includes. */
#include <FreeRTOS.h>

#include "FreeRTOS_CLI.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"
#include "hw_config.h"
#include "stdio_cli.h"

extern int low_level_io_tests();
extern void vMultiTaskStdioWithCWDTest(const char *const pcMountPath,
                                       uint16_t usStackSizeWords);

/*
 * Functions used to create and then test files on a disk.
 */
extern void vStdioWithCWDTest(const char *pcMountPath);
extern void vMultiTaskStdioWithCWDTest(const char *const pcMountPath,
                                       uint16_t usStackSizeWords);
extern void big_file_test(const char *const pathname, size_t size,
                          uint32_t seed);

static void ls() {
    char pcWriteBuffer[128] = {0};

    FF_FindData_t xFindStruct;
    memset(&xFindStruct, 0x00, sizeof(FF_FindData_t));

    ff_getcwd(pcWriteBuffer, sizeof(pcWriteBuffer));
    FF_PRINTF("Directory Listing: %s\n", pcWriteBuffer);

    int iReturned = ff_findfirst("", &xFindStruct);
    if (FF_ERR_NONE != iReturned) {
        FF_PRINTF("ff_findfirst error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  -stdioGET_ERRNO());
        return;
    }
    do {
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;

        /* Point pcAttrib to a string that describes the file. */
        if ((xFindStruct.ucAttributes & FF_FAT_ATTR_DIR) != 0) {
            pcAttrib = pcDirectory;
        } else if (xFindStruct.ucAttributes & FF_FAT_ATTR_READONLY) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        FF_PRINTF("%s [%s] [size=%d]\n", xFindStruct.pcFileName, pcAttrib,
                  (int)xFindStruct.ulFileSize);
    } while (FF_ERR_NONE == ff_findnext(&xFindStruct));
}

// Maximum number of elements in buffer
#define BUFFER_MAX_LEN 10

// Adapted from "FATFileSystem example"
//  at https://os.mbed.com/docs/mbed-os/v5.15/apis/fatfilesystem.html
static void prvSimpleTest() {
    int err = 0;

    FF_PRINTF("\nSimple Test\n");

    // Open the numbers file
    FF_PRINTF("Opening \"/fs/numbers.txt\"... ");
    FF_FILE *f = ff_fopen("/fs/numbers.txt", "r+");
    FF_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
    fflush(stdout);
    if (!f) {
        // Create the numbers file if it doesn't exist
        FF_PRINTF("No file found, creating a new file... ");
        fflush(stdout);
        f = ff_fopen("/fs/numbers.txt", "w+");
        FF_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
        fflush(stdout);
        if (!f) {
            FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                      -stdioGET_ERRNO());
            return;
        }
        for (int i = 0; i < 10; i++) {
            FF_PRINTF("\rWriting numbers (%d/%d)... ", i, 10);
            fflush(stdout);
            err = ff_fprintf(f, "    %d\n", i);
            if (err < 0) {
                FF_PRINTF("Fail :(\n");
                FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                          -stdioGET_ERRNO());
            }
        }
        FF_PRINTF("\rWriting numbers (%d/%d)... OK\n", 10, 10);
        fflush(stdout);

        FF_PRINTF("Seeking file... ");
        err = ff_fseek(f, 0, FF_SEEK_SET);
        FF_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
        fflush(stdout);
        if (err < 0) {
            FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                      -stdioGET_ERRNO());
        }
    }
    // Go through and increment the numbers
    for (int i = 0; i < 10; i++) {
        FF_PRINTF("\nIncrementing numbers (%d/%d)... ", i, 10);

        // Get current stream position
        long pos = ff_ftell(f);

        // Parse out the number and increment
        char buf[BUFFER_MAX_LEN];
        if (!ff_fgets(buf, BUFFER_MAX_LEN, f)) {
            FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                      -stdioGET_ERRNO());
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
        ff_fseek(f, pos, FF_SEEK_SET);

        // Store number
        ff_fprintf(f, "    %d\n", (int)number);

        // Flush between write and read on same file
        //         ff_fflush(f);
    }
    FF_PRINTF("\rIncrementing numbers (%d/%d)... OK\n", 10, 10);
    fflush(stdout);

    // Close the file which also flushes any cached writes
    FF_PRINTF("Closing \"/fs/numbers.txt\"... ");
    err = ff_fclose(f);
    FF_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
    fflush(stdout);
    if (err < 0) {
        FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  -stdioGET_ERRNO());
    }

    ls();

    err = ff_chdir("fs");
    if (err < 0) {
        FF_PRINTF("chdir error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  -stdioGET_ERRNO());
    }

    ls();

    // Display the numbers file
    FF_PRINTF("Opening \"/fs/numbers.txt\"... ");
    f = ff_fopen("/fs/numbers.txt", "r");
    FF_PRINTF("%s\n", (!f ? "Fail :(" : "OK"));
    fflush(stdout);
    if (!f) {
        FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  -stdioGET_ERRNO());
    }

    FF_PRINTF("numbers:\n");
    while (!ff_feof(f)) {
        int c = ff_fgetc(f);
        FF_PRINTF("%c", c);
    }

    FF_PRINTF("\nClosing \"/fs/numbers.txt\"... ");
    err = ff_fclose(f);
    FF_PRINTF("%s\n", (err < 0 ? "Fail :(" : "OK"));
    fflush(stdout);
    if (err < 0) {
        FF_PRINTF("error: %s (%d)\n", strerror(stdioGET_ERRNO()),
                  -stdioGET_ERRNO());
    }
}

/*-----------------------------------------------------------*/
static BaseType_t runFormat(char *pcWriteBuffer, size_t xWriteBufferLen,
                            const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);
    FF_Disk_t *pxDisk = NULL;
    bool rc = format(&pxDisk, pcParameter);
    if (!rc)
        FF_PRINTF("Format failed!\n");
    else {
        //    	/*Unmount the partition. */
        //    	FF_Error_t xError = FF_SDDiskUnmount(pxDisk);
        //        if (FF_isERR( xError ) != pdFALSE) {
        //        	FF_PRINTF("FF_Unmount: %s\n", (const char *)
        //        FF_GetErrMessage(xError));
        //        }
        //    	FF_SDDiskDelete(pxDisk);
    }
    return pdFALSE;
}
static const CLI_Command_Definition_t xFormat = {
    "format", /* The command string to type. */
    "\nformat <device name>:\n Format <device name>\n"
    "\te.g.: \"format sd0\"\n",
    runFormat, /* The function to run. */
    1          /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runMount(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    char buf[cmdMAX_INPUT_SIZE];
    snprintf(buf, cmdMAX_INPUT_SIZE, "/%s", pcParameter);  // Add '/' for path
    FF_Disk_t *pxDisk = NULL;
    bool rc = mount(&pxDisk, pcParameter, buf);
    if (!rc) FF_PRINTF("Mount failed!\n");

    return pdFALSE;
}
static const CLI_Command_Definition_t xMount = {
    "mount", /* The command string to type. */
    "\nmount <device name>:\n Mount <device name> at /<device name>\n"
    "\te.g.: \"mount sd0\"\n",
    runMount, /* The function to run. */
    1         /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runEject(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    eject(pcParameter);

    return pdFALSE;
}
static const CLI_Command_Definition_t xEject = {
    "eject", /* The command string to type. */
    "\neject <device name>:\n Eject <device name>\n"
    "\te.g.: \"eject sd0\"\n",
    runEject, /* The function to run. */
    1         /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runLLIOTCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                  const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    low_level_io_tests(pcParameter);

    return pdFALSE;
}
static const CLI_Command_Definition_t xLowLevIOTests = {
    "lliot", /* The command string to type. */
    "\nlliot <device name>:\n !DESTRUCTIVE! Low Level I/O Driver Test\n"
    "\te.g.: \"lliot sd0\"\n",
    runLLIOTCommand, /* The function to run. */
    1                /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
void vCreateAndVerifyExampleFiles(
    const char *pcMountPath);  // in CreateAndVerifyExampleFiles.c

static BaseType_t runCreateDiskAndExampleFiles(char *pcWriteBuffer,
                                               size_t xWriteBufferLen,
                                               const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);
    char buf[cmdMAX_INPUT_SIZE];
    snprintf(buf, cmdMAX_INPUT_SIZE, "/%s", pcParameter);  // Add '/' for path
    FF_Disk_t *pxDisk = NULL;
    bool rc = mount(&pxDisk, pcParameter, buf);
    configASSERT(rc);

    vCreateAndVerifyExampleFiles(buf);

    // unmount(pxDisk, buf);
    return pdFALSE;
}
static const CLI_Command_Definition_t xExampFiles = {
    "cdef", /* The command string to type. */
    "\ncdef <device name>:\n Create Disk and Example Files\n"
    "Expects card to be already formatted but not mounted.\n"
    "\te.g.: \"cdef sd0\"\n",
    runCreateDiskAndExampleFiles, /* The function to run. */
    1                             /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runStdioWithCWDTest(char *pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);
    char buf[cmdMAX_INPUT_SIZE];
    snprintf(buf, cmdMAX_INPUT_SIZE, "/%s", pcParameter);  // Add '/' for path
    FF_Disk_t *pxDisk = NULL;
    bool rc = mount(&pxDisk, pcParameter, buf);
    configASSERT(rc);

    // Clear out leftovers from earlier runs
    ff_chdir(buf);
    ff_remove("Dummy.txt");
    ff_deltree("source_dir");
    ff_deltree("destination_dir");

    vStdioWithCWDTest(buf);

    // unmount(pxDisk, buf);
    return pdFALSE;
}
static const CLI_Command_Definition_t xStdioWithCWDTest = {
    "swcwdt", /* The command string to type. */
    "\nswcwdt <device name>:\n Stdio With CWD Test\n"
    "Expects card to be already formatted but not mounted.\n"
    "Note: run cdef first!"
    "\te.g.: \"swcwdt sd0\"\n",
    runStdioWithCWDTest, /* The function to run. */
    1                    /* No parameters are expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runMultiTaskStdioWithCWDTest(char *pcWriteBuffer,
                                               size_t xWriteBufferLen,
                                               const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);
    char buf[cmdMAX_INPUT_SIZE];
    snprintf(buf, cmdMAX_INPUT_SIZE, "/%s", pcParameter);  // Add '/' for path
    FF_Disk_t *pxDisk = NULL;
    bool rc = mount(&pxDisk, pcParameter, buf);
    configASSERT(rc);

    // Clear out leftovers from earlier runs
    size_t i;
    for (i = 0; i <= 4; ++i) {
        snprintf(buf, cmdMAX_INPUT_SIZE, "/%s/%u", pcParameter, i);
        ff_deltree(buf);
    }
    snprintf(buf, cmdMAX_INPUT_SIZE, "/%s", pcParameter);  // Add '/' for path
    vMultiTaskStdioWithCWDTest(buf, 1024);

    return pdFALSE;
}
static const CLI_Command_Definition_t xMultiTaskStdioWithCWDTest = {
    "mtswcwdt", /* The command string to type. */
    "\nmtswcwdt <device name>:\n MultiTask Stdio With CWD Test\n"
    "Expects card to be already formatted but not mounted.\n"
    "\te.g.: \"mtswcwdt sd0\"\n",
    runMultiTaskStdioWithCWDTest, /* The function to run. */
    1                             /* parameters are expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t runSimpleFSTest(char *pcWriteBuffer, size_t xWriteBufferLen,
                                  const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    FF_Disk_t *pxDisk = NULL;
    bool rc = mount(&pxDisk, pcParameter, "/fs");
    configASSERT(rc);

    prvSimpleTest();

    //	unmount(pxDisk, "/fs");
    return pdFALSE;
}
static const CLI_Command_Definition_t xSimpleFSTest = {
    "simple", /* The command string to type. */
    "\nsimple <device name>:\n Run simple FS tests\n"
    "Expects card to already be formatted but not mounted.\n"
    "\te.g.: \"simple sd0\"\n",
    runSimpleFSTest, /* The function to run. */
    1                /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
static BaseType_t prvBigFileTest(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    char *pcPathName;
    const char *pcSeed, *pcSize;
    size_t size;
    uint32_t seed;
    BaseType_t xParameterStringLength;

    /* Obtain the seed. */
    pcSeed = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        3,                      /* Return the third parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcSeed);
    seed = atoi(pcSeed);

    /* Obtain the file size. */
    pcSize = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        2,                      /* Return the second parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcSize);
    size = strtoul(pcSize, 0, 0);

    /* Obtain the pathname. */
    pcPathName = (char *)FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcPathName);

    /* Terminate the string. */
    pcPathName[xParameterStringLength] = 0x00;

    big_file_test(pcPathName, size, seed);

    return pdFALSE;
}
/* Structure that defines the COPY command line command, which deletes a file.
 */
static const CLI_Command_Definition_t xBFT = {
    "big_file_test", /* The command string to type. */
    "\r\nbig_file_test <pathname> <size in bytes> <seed>:\n"
    " Writes random data to file <pathname>.\n"
    " <size in bytes> must be multiple of 512.\n"
    "\te.g.: big_file_test sd0/bf 1048576 1\n"
    "\tor: big_file_test sd0/big3G-3 0xC0000000 3\n",
    prvBigFileTest, /* The function to run. */
    3               /* Two parameters are expected. */
};
/*-----------------------------------------------------------*/

void register_fs_tests() {
    /* Register all the command line commands defined immediately above. */
    extern const CLI_Command_Definition_t xMTLowLevIOTests;

    FreeRTOS_CLIRegisterCommand(&xFormat);
    FreeRTOS_CLIRegisterCommand(&xMount);
    FreeRTOS_CLIRegisterCommand(&xEject);
    FreeRTOS_CLIRegisterCommand(&xLowLevIOTests);
    FreeRTOS_CLIRegisterCommand(&xMTLowLevIOTests);
    FreeRTOS_CLIRegisterCommand(&xSimpleFSTest);
    FreeRTOS_CLIRegisterCommand(&xExampFiles);
    FreeRTOS_CLIRegisterCommand(&xStdioWithCWDTest);
    FreeRTOS_CLIRegisterCommand(&xMultiTaskStdioWithCWDTest);
    FreeRTOS_CLIRegisterCommand(&xBFT);
}

/* [] END OF FILE */
