
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "hardware/adc.h"
#include "hardware/clocks.h" 
#include "hardware/rtc.h"
#include "pico/stdlib.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */
//
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"
#include "tests/tests.h"
// #include "util.h" // for wiring test

#ifndef USE_PRINTF
#error This program is useless without standard input and output.
#endif

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;

#pragma GCC diagnostic ignored "-Wunused-function"
#ifdef NDEBUG 
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static sd_card_t *sd_get_by_name(const char *const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
static FATFS *sd_get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
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

    datetime_t t = {.year = static_cast<int16_t>(year),
                    .month = static_cast<int8_t>(month),
                    .day = static_cast<int8_t>(date),
                    .dotw = 0,  // 0 is Sunday, so 5 is Friday
                    .hour = static_cast<int8_t>(hour),
                    .min = static_cast<int8_t>(min),
                    .sec = static_cast<int8_t>(sec)};
    rtc_set_datetime(&t);
}
static void run_date() {
    char buf[128] = {0};
    time_t epoch_secs = time(NULL);
    struct tm *ptm = localtime(&epoch_secs);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    assert(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}
static void run_lliot() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 != sd_get_num()) {
        printf("Missing argument: Specify physical drive number\n");
        return;
    }    
    size_t pnum = 0;
    if (arg1) {
        pnum = strtoul(arg1, NULL, 0);
    }
    lliot(pnum);
}
static void run_format() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 == sd_get_num()) 
        arg1 = sd_get_by_num(0)->pcName;
    else if (!arg1) {
        printf("Missing argument: Specify logical drive\n");
        return;
    }    
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mount() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 == sd_get_num()) 
        arg1 = sd_get_by_num(0)->pcName;
    else if (!arg1) {
        printf("Missing argument: Specify logical drive\n");
        return;
    }    
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    assert(pSD);
    pSD->mounted = true;
}
static void run_unmount() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 == sd_get_num()) 
        arg1 = sd_get_by_num(0)->pcName;
    else if (!arg1) {
        printf("Missing argument: Specify logical drive\n");
        return;
    }    
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr) {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    assert(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
}
static void run_chdrive() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1) arg1 = sd_get_by_num(0)->pcName;
    FRESULT fr = f_chdrive(arg1);
    if (FR_OK != fr) printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_getfree() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 == sd_get_num()) 
        arg1 = sd_get_by_num(0)->pcName;
    else if (!arg1) {
        printf("Missing argument: Specify logical drive\n");
        return;
    }    
    /* Get volume information and free clusters of drive */
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    DWORD fre_clust, fre_sect, tot_sect;
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
    const char *arg1 = strtok(NULL, " ");
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
static void run_bench() {
    const char *arg1 = strtok(NULL, " ");
    if (!arg1 && 1 == sd_get_num()) 
        arg1 = sd_get_by_num(0)->pcName;
    else if (!arg1) {
        printf("Missing argument: Specify logical drive\n");
        return;
    }    
    bench(arg1);
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
    next_log_time = delayed_by_ms(get_absolute_time(), period);
}
static void run_stop_logger() { logger_enabled = false; }

/* Derived from pico-examples/clocks/hello_48MHz/hello_48MHz.c */
static void run_measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\treported  = %lukHz\n", f_clk_sys, clock_get_hz(clk_sys)/KHZ);
    printf("clk_peri = %dkHz\treported  = %lukHz\n", f_clk_peri, clock_get_hz(clk_peri)/KHZ);
    printf("clk_usb  = %dkHz\treported  = %lukHz\n", f_clk_usb, clock_get_hz(clk_usb)/KHZ);
    printf("clk_adc  = %dkHz\treported  = %lukHz\n", f_clk_adc, clock_get_hz(clk_adc)/KHZ);
    printf("clk_rtc  = %dkHz\treported  = %lukHz\n", f_clk_rtc, clock_get_hz(clk_rtc)/KHZ);

    // Can't measure clk_ref / xosc as it is the ref
}
static void run_set_sys_clock_48mhz() {
    set_sys_clock_48mhz();
    setup_default_uart();
}
static void run_set_sys_clock_khz() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    int khz = atoi(arg1);

    bool configured = set_sys_clock_khz(khz, false);
    if (!configured) {
        printf("Not possible. Clock not configured.\n");
        return;
    }
    /*
    By default, when reconfiguring the system clock PLL settings after runtime initialization,
    the peripheral clock is switched to the 48MHz USB clock to ensure continuity of peripheral operation.
    There seems to be a problem with running the SPI 2.4 times faster than the system clock, 
    even at the same SPI baud rate. 
    Anyway, for now, reconfiguring the peripheral clock to the system clock at its new frequency works OK.
    */
    bool ok = clock_configure(clk_peri,
                              0,
                              CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                              clock_get_hz(clk_sys),
                              clock_get_hz(clk_sys));
    assert(ok);

    setup_default_uart();
}

static void clr() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    int gp = atoi(arg1);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 0);
}

static void set() {
    char *arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Missing argument\n");
        return;
    }
    int gp = atoi(arg1);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 1);
}

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
    {"bench", run_bench, "bench <drive#:>:\n  A simple binary write/read benchmark"},
    {"big_file_test", run_big_file_test,
     "big_file_test <pathname> <size in MiB> <seed>:\n"
     " Writes random data to file <pathname>.\n"
     " Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)\n"
     "\te.g.: big_file_test bf 1 1\n"
     "\tor: big_file_test big3G-3 3072 3"},
    {"cdef", run_cdef,
     "cdef:\n  Create Disk and Example Files\n"
     "  Expects card to be already formatted and mounted"},
    {"swcwdt", run_swcwdt,
     "swcwdt:\n Stdio With CWD Test\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: run cdef first!"},
    {"loop_swcwdt", run_loop_swcwdt,
     "loop_swcwdt:\n Run Create Disk and Example Files and Stdio With CWD "
     "Test in a loop.\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: Type any key to quit."},
    {"start_logger", run_start_logger,
     "start_logger:\n"
     "  Start Data Log Demo"},
    {"stop_logger", run_stop_logger,
     "stop_logger:\n"
     "  Stop Data Log Demo"},
    // Clocks testing:
    // {"set_sys_clock_48mhz", run_set_sys_clock_48mhz, "set_sys_clock_48mhz:\n"
    //  "  Set the system clock to 48MHz"},
    // {"set_sys_clock_khz", run_set_sys_clock_khz, "set_sys_clock_khz <khz>:\n"
    //  "  Set the system clock system clock frequency in khz."},
    // {"measure_freqs", run_measure_freqs, "measure_freqs:\n"
    //  "  Count the RP2040 clock frequencies and report."},
    // {"clr", clr, "clr <gpio #>: clear a GPIO"},
    // {"set", set, "set <gpio #>: set a GPIO"},

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

// If the card is physically removed, unmount the filesystem:
static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;
    if (busy) return; // Avoid switch bounce
    busy = true;
    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *pSD = sd_get_by_num(i);
        if (pSD->card_detect_gpio == gpio) {
            if (pSD->mounted) {
                DBG_PRINTF("(Card Detect Interrupt: unmounting %s)\n", pSD->pcName);
                FRESULT fr = f_unmount(pSD->pcName);
                if (FR_OK == fr) {
                    pSD->mounted = false;
                } else {
                    printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
                }
            }
            pSD->m_Status |= STA_NOINIT; // in case medium is removed
            sd_card_detect(pSD);
        }
    }
    busy = false;
}

int main() {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 1); // specify that the stream should be unbuffered
    time_init();
    adc_init();

    printf("\033[2J\033[H");  // Clear Screen
    printf("\n> ");
    stdio_flush();

    // Implicitly called by disk_initialize, 
    // but called here to set up the GPIOs 
    // before enabling the card detect interrupt:
    sd_init_driver();

    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *pSD = sd_get_by_num(i);
        if (pSD->use_card_detect) {
            // Set up an interrupt on Card Detect to detect removal of the card
            // when it happens:
            gpio_set_irq_enabled_with_callback(
                pSD->card_detect_gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                true, &card_detect_callback);
        }
    }

    for (;;) {  // Super Loop
        if (logger_enabled &&
            absolute_time_diff_us(get_absolute_time(), next_log_time) < 0) {
            if (!process_logger()) logger_enabled = false;
            next_log_time = delayed_by_ms(next_log_time, period);
        }
        int cRxedChar = getchar_timeout_us(0);
        /* Get the character from terminal */
        if (PICO_ERROR_TIMEOUT != cRxedChar) process_stdio(cRxedChar);
    }
    return 0;
}
