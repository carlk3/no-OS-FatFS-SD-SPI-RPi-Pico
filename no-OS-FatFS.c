#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "pico/stdlib.h"
//
#include "ff.h"
#include "ff_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

extern void ls();
extern void simple();
extern void lliot();

static FATFS fs;

static void test() {
    printf("test\n");
    char *p = strtok(NULL, " ");
    if (p) printf("arg1: %s\n", p);
}
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
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mount() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        //printf("Missing argument\n");
        //return;
        arg1 = "";
    }
    FRESULT fr = f_mount(&fs, arg1, 1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_unmount() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        //printf("Missing argument\n");
        //return;
        arg1 = "";
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_help();

typedef void (*p_fn_t)();
typedef struct {
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"test", test, "help for test"},
    {
        "setrtc",
        run_setrtc,
        "setrtc <DD> <MM> <YY> <hh> <mm> <ss>:\n"
        "  Set Real Time Clock\n"
        "  Parameters: new date (DD MM YY) new time in 24-hour format "
        "(hh mm ss)\n"
        "\te.g.:setrtc 16 3 21 0 4 0",
    },
    {"date", run_date, "date:\n Print current date and time"},
    {
        "lliot",
        lliot,
        "lliot <device name>:\n !DESTRUCTIVE! Low Level I/O Driver Test\n"
        "\te.g.: \"lliot sd0\"",
    },
    {
        "format",
        run_format,
        "format <drive#:>:\n"
        "  Creates an FAT/exFAT volume on the logical drive.\n"
        "\te.g.: \"format 0:\"",
    },
    {
        "mount",
        run_mount,
        "mount <drive#:>:\n"
        "  Register the work area of the volume\n"
        "\te.g.: \"mount 0:\"",
    },
    {"unmount", run_unmount, "unmount <drive#:>:\n  Unregister the work area of the volume\n"},
    {"ls", ls, "ls:\n  List directory"},
    {"simple", simple, "simple:\n  Run simple FS tests"},
    {"help", run_help, "Shows this command help"}};
static void run_help() {
    for (size_t i = 0; i < count_of(cmds); ++i) {
        printf("%s\n\n", cmds[i].help);
    }
}

int main() {
    stdio_init_all();
    time_init();

    printf("\033[2J\033[H");  // Clear Screen
    printf("%s", "\n> ");
    stdio_flush();

    // sd_init(sd_get_by_num(0));

    char cmd[256] = {0};
    size_t ix = 0;

    for (;;) {
        int cRxedChar = getchar_timeout_us(1000 * 1000);
        /* Get the character from terminal */
        if (PICO_ERROR_TIMEOUT == cRxedChar) continue;
        if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
            '\b' != cRxedChar && cRxedChar != (char)127)
            continue;
        printf("%c", cRxedChar);  // echo
        stdio_flush();
        if (cRxedChar == '\r') {
            /* Just to space the output from the input. */
            printf("%c", '\n');
            stdio_flush();

            if (!strnlen(cmd, sizeof cmd)) {  // Empty input
                printf("%s", "> ");
                stdio_flush();
                continue;
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
                if (count_of(cmds) == i)
                    printf("Command \"%s\" not found\n", cmdn);
            }
            ix = 0;
            memset(cmd, 0, sizeof cmd);
            printf("%s", "\n> ");
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
    return 0;
}
