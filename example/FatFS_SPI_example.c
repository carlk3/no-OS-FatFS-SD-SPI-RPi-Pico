#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "hardware/adc.h"
#include "pico/stdlib.h"
//
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

extern void lliot();
extern void simple();
extern void big_file_test(const char *const pathname, size_t size,
                          uint32_t seed);
extern void vCreateAndVerifyExampleFiles(const char *pcMountPath);
extern void vStdioWithCWDTest(const char *pcMountPath);
extern bool process_logger();

typedef struct {
    FATFS fatfs;
    char const *const name;
} fatfs_dscr_t;
static fatfs_dscr_t fatfs_dscrs[2] = {{.name = "0:"}, {.name = "1:"}};
static FATFS *get_fs_by_name(const char *name) {
    for (size_t i = 0; i < count_of(fatfs_dscrs); ++i) {
        if (0 == strcmp(fatfs_dscrs[i].name, name)) {
            return &fatfs_dscrs[i].fatfs;
        }
    }
    return NULL;
}

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_time;

static void run_setrtc() {
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr) {
        printf("Missing argument\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr) {
        printf("Missing argument\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr) {
        printf("Missing argument\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr) {
        printf("Missing argument\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr) {
        printf("Missing argument\n");
        return;
    };
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr) {
        printf("Missing argument\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {.year = year,
                    .month = month,
                    .day = date,
                    .dotw = 0,  // 0 is Sunday, so 5 is Friday
                    .hour = hour,
                    .min = min,
                    .sec = sec};
    // bool r = rtc_set_datetime(&t);
    setrtc(&t);
}
static void run_lliot() {
    size_t pnum = 0;
    char *arg1 = strtok(NULL, " ");
    if (arg1) {
        pnum = strtoul(arg1, NULL, 0);
    }
    lliot(pnum);
}
static void run_date() {
    char buf[128] = {0};
    time_t epoch_secs = time(NULL);
    struct tm *ptm = localtime(&epoch_secs);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    myASSERT(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}
static void run_format() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    FATFS *p_fs = get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mount() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = "0:";
    FATFS *p_fs = get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_unmount() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = "";
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_chdrive() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = "0:";
    FRESULT fr = f_chdrive(arg1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_getfree() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = "0:";
    DWORD fre_clust, fre_sect, tot_sect;
    /* Get volume information and free clusters of drive */
    FATFS *p_fs = get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr) {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    /* Get total sectors and free sectors */
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    /* Print the free space (assuming 512 bytes/sector) */
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2,
           fre_sect / 2);
}
static void run_cd() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    FRESULT fr = f_chdir(arg1);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mkdir() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    FRESULT fr = f_mkdir(arg1);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
void ls(const char *dir) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;      /* Directory object */
    FILINFO fno; /* File information */
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
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
static void run_ls() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = "";
    ls(arg1);
}
static void run_cat() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil)) {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr) printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_big_file_test() {
    const char *pcPathName = strtok(NULL, " ");
    if (!pcPathName) {
        printf("Missing argument\n");
        return;
    }
    const char *pcSize = strtok(NULL, " ");
    if (!pcSize) {
        printf("Missing argument\n");
        return;
    }
    size_t size = strtoul(pcSize, 0, 0);
    const char *pcSeed = strtok(NULL, " ");
    if (!pcSeed) {
        printf("Missing argument\n");
        return;
    }
    uint32_t seed = atoi(pcSeed);
    big_file_test(pcPathName, size, seed);
}
static void del_node(const char *path) {
    FILINFO fno;
    char buff[256];
    /* Directory to be deleted */
    strlcpy(buff, path, sizeof(buff));
    /* Delete the directory */
    FRESULT fr = delete_node(buff, sizeof buff / sizeof buff[0], &fno);
    /* Check the result */
    if (fr) {
        printf("Failed to delete the directory %s. ", path);
        printf("%s error: %s (%d)\n", __func__, FRESULT_str(fr), fr);
    }
}
static void run_del_node() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    del_node(arg1);
}
static void run_cdef() {
    f_mkdir("/cdef");  // fake mountpoint
    vCreateAndVerifyExampleFiles("/cdef");
}
static void run_swcwdt() { vStdioWithCWDTest("/cdef"); }
static void run_loop_swcwdt() {
    int cRxedChar = 0;
    do {
        del_node("/cdef");
        run_cdef();
        run_swcwdt();
        cRxedChar = getchar_timeout_us(0);
    } while (PICO_ERROR_TIMEOUT == cRxedChar);
}
static void run_start_logger() {
    logger_enabled = true;
    next_time = delayed_by_ms(get_absolute_time(), period);
}
static void run_stop_logger() { logger_enabled = false; }
static void run_help();

typedef void (*p_fn_t)();
typedef struct {
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc,
     "setrtc <DD> <MM> <YY> <hh> <mm> <ss>:\n"
     "  Set Real Time Clock\n"
     "  Parameters: new date (DD MM YY) new time in 24-hour format "
     "(hh mm ss)\n"
     "\te.g.:setrtc 16 3 21 0 4 0"},
    {"date", run_date, "date:\n Print current date and time"},
    {"lliot", run_lliot,
     "lliot <drive#>:\n !DESTRUCTIVE! Low Level I/O Driver Test\n"
     "\te.g.: lliot 1"},
    {"format", run_format,
     "format [<drive#:>]:\n"
     "  Creates an FAT/exFAT volume on the logical drive.\n"
     "\te.g.: format 0:"},
    {"mount", run_mount,
     "mount [<drive#:>]:\n"
     "  Register the work area of the volume\n"
     "\te.g.: mount 0:"},
    {"unmount", run_unmount,
     "unmount <drive#:>:\n"
     "  Unregister the work area of the volume"},
    {"chdrive", run_chdrive,
     "chdrive <drive#:>:\n"
     "  Changes the current directory of the logical drive.\n"
     "  <path> Specifies the directory to be set as current directory.\n"
     "\te.g.: chdrive 1:"},
    {"getfree", run_getfree,
     "getfree [<drive#:>]:\n"
     "  Print the free space on drive"},
    {"cd", run_cd,
     "cd <path>:\n"
     "  Changes the current directory of the logical drive.\n"
     "  <path> Specifies the directory to be set as current directory.\n"
     "\te.g.: cd /dir1"},
    {"mkdir", run_mkdir,
     "mkdir <path>:\n"
     "  Make a new directory.\n"
     "  <path> Specifies the name of the directory to be created.\n"
     "\te.g.: mkdir /dir1"},
    {"del_node", run_del_node,
     "del_node <path>:\n"
     "  Remove directory and all of its contents.\n"
     "  <path> Specifies the name of the directory to be deleted.\n"
     "\te.g.: del_node /dir1"},
    {"ls", run_ls, "ls:\n  List directory"},
    {"cat", run_cat, "cat <filename>:\n  Type file contents"},
    {"simple", simple, "simple:\n  Run simple FS tests"},
    {"big_file_test", run_big_file_test,
     "big_file_test <pathname> <size in bytes> <seed>:\n"
     " Writes random data to file <pathname>.\n"
     " <size in bytes> must be multiple of 512.\n"
     "\te.g.: big_file_test bf 1048576 1\n"
     "\tor: big_file_test big3G-3 0xC0000000 3"},
    {"cdef", run_cdef,
     "cdef:\n  Create Disk and Example Files\n"
     "  Expects card to be already formatted and mounted"},
    {"swcwdt", run_swcwdt,
     "\nswcwdt:\n Stdio With CWD Test\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: run cdef first!"},
    {"loop_swcwdt", run_loop_swcwdt,
     "\nloop_swcwdt:\n Run Create Disk and Example Files and Stdio With CWD "
     "Test in a loop.\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: Type any key to quit."},
    {"start_logger", run_start_logger,
     "start_logger:\n"
     "  Start Data Log Demo"},
    {"stop_logger", run_stop_logger,
     "stop_logger:\n"
     "  Stop Data Log Demo"},
    {"help", run_help,
     "help:\n"
     "  Shows this command help."}};
static void run_help() {
    for (size_t i = 0; i < count_of(cmds); ++i) {
        printf("%s\n\n", cmds[i].help);
    }
}

static void process_stdio(int cRxedChar) {
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar);  // echo
    stdio_flush();
    if (cRxedChar == '\r') {
        /* Just to space the output from the input. */
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd)) {  // Empty input
            printf("> ");
            stdio_flush();
            return;
        }
        /* Process the input string received prior to the newline. */
        char *cmdn = strtok(cmd, " ");
        if (cmdn) {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i) {
                if (0 == strcmp(cmds[i].command, cmdn)) {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i) printf("Command \"%s\" not found\n", cmdn);
        }
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    } else {  // Not newline
        if (cRxedChar == '\b' || cRxedChar == (char)127) {
            /* Backspace was pressed.  Erase the last character
             in the string - if any. */
            if (ix > 0) {
                ix--;
                cmd[ix] = '\0';
            }
        } else {
            /* A character was entered.  Add it to the string
             entered so far.  When a \n is entered the complete
             string will be passed to the command interpreter. */
            if (ix < sizeof cmd - 1) {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}

int main() {
    stdio_init_all();
    time_init();
    adc_init();
    sd_init_driver();

    printf("\033[2J\033[H");  // Clear Screen
    printf("\n> ");
    stdio_flush();

    for (;;) {
        int cRxedChar = getchar_timeout_us(1000);
        /* Get the character from terminal */
        if (PICO_ERROR_TIMEOUT != cRxedChar) process_stdio(cRxedChar);
        if (logger_enabled &&
            absolute_time_diff_us(get_absolute_time(), next_time) < 0) {
            if (!process_logger()) logger_enabled = false;
            next_time = delayed_by_ms(next_time, period);
        }
    }
    return 0;
}
