/* Ported from: https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino
 *
 * This program is a simple binary write/read benchmark.
 *
 * Warning: this might destroy any data on the SD card(s), depending on configuration.
 */

#include <stdio.h>

#include "Arduino.h"
#include "FatFsSd.h"

using namespace FatFsNs;

/* Infrastructure*/
#if 1
// This implementation has the advantage that it works for
//   printfs elsewhere in the system; e.g., in an assert
//   somewhere in the runtime.
extern "C" int printf(const char* __restrict format, ...) {
    char buf[256] = {0};
    va_list xArgs;
    va_start(xArgs, format);
    vsnprintf(buf, sizeof buf, format, xArgs);
    va_end(xArgs);
    return Serial1.write(buf);
}
extern "C" int puts(const char* s) {
    return Serial1.write(s);
}
#else
#define printf Serial1.printf
#define puts Serial1.println
#endif

#define error(s)                  \
    {                             \
        printf("ERROR: %s\n", s); \
        for (;;) __breakpoint();  \
    }

/* ********************************************************************** */

// Whether or not to format the card(s) in setup():
static const bool FORMAT = true;

// Set PRE_ALLOCATE true to pre-allocate file clusters.
static const bool PRE_ALLOCATE = true;

// Set SKIP_FIRST_LATENCY true if the first read/write to the SD can
// be avoid by writing a file header or reading the first record.
static const bool SKIP_FIRST_LATENCY = true;

// Size of read/write.
// static const size_t BUF_SIZE = 512;
#define BUF_SIZE (20 * 1024)

// File size in MiB where MiB = 1048576 bytes.
static const uint32_t FILE_SIZE_MiB = 5;

// Write pass count.
static const uint8_t WRITE_COUNT = 2;

// Read pass count.
static const uint8_t READ_COUNT = 2;
//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------
// File size in bytes.
// static const uint32_t FILE_SIZE = 1000000UL * FILE_SIZE_MB;
static const uint32_t FILE_SIZE = (1024 * 1024 * FILE_SIZE_MiB);

static void cidDmp(SdCard* SdCard_p) {
    cid_t cid;
    if (!SdCard_p->readCID(&cid)) {
        error("readCID failed");
    }
    printf("\nManufacturer ID: ");
    printf("0x%x\n", cid.mid);
    printf("OEM ID: ");
    printf("%c%c\n", cid.oid[0], cid.oid[1]);
    printf("Product: ");
    for (uint8_t i = 0; i < 5; i++) {
        printf("%c", cid.pnm[i]);
    }
    printf("\nRevision: ");
    printf("%d.%d\n", CID_prvN(&cid), CID_prvM(&cid));
    printf("Serial number: ");
    printf("0x%lx\n", CID_psn(&cid));
    printf("Manufacturing date: ");
    printf("%d/%d\n", CID_mdtMonth(&cid), CID_mdtYear(&cid));
    printf("\n");
}

// First (if s is not NULL and *s is not a null byte ('\0')) the argument string s is printed,
//   followed by a colon and a blank. Then the FRESULT error message and a new-line.
static void chk_result(const char* s, FRESULT fr) {
    if (FR_OK != fr) {
        if (s && *s)
            printf("%s: %s (%d)\n", s, FRESULT_str(fr), fr);
        else
            printf("%s (%d)\n", FRESULT_str(fr), fr);
        for (;;) __breakpoint();
    }
}

void setup() {
    // put your setup code here, to run once:

    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;                     // Serial is via USB; wait for enumeration
    printf("\033[2J\033[H");  // Clear Screen
    printf("\nUse a freshly formatted SD for best performance.\n");

    /*
    This example assumes the following wiring for SD card 0:

        | signal | SPI1 | GPIO | card | Description            |
        | ------ | ---- | ---- | ---- | ---------------------- |
        | MISO   | RX   | 12   |  DO  | Master In, Slave Out   |
        | CS0    | CSn  | 09   |  CS  | Slave (or Chip) Select |
        | SCK    | SCK  | 14   | SCLK | SPI clock              |
        | MOSI   | TX   | 15   |  DI  | Master Out, Slave In   |
        | CD     |      | 13   |  DET | Card Detect            |

    This example assumes the following wiring for SD card 1:

        | GPIO  |  card | Function    |
        | ----  |  ---- | ----------- |
        |  16   |  DET  | Card Detect |
        |  17   |  CLK  | SDIO_CLK    |
        |  18   |  CMD  | SDIO_CMD    |
        |  19   |  DAT0 | SDIO_D0     |
        |  20   |  DAT1 | SDIO_D1     |
        |  21   |  DAT2 | SDIO_D2     |
        |  22   |  DAT3 | SDIO_D3     |
    */
    /* Hardware Configuration of SPI object */

    // GPIO numbers, not Pico pin numbers!
    SpiCfg spi(
        spi1,             // spi_inst_t *hw_inst,
        12,               // uint miso_gpio,
        15,               // uint mosi_gpio,
        14,               // uint sck_gpio
        25 * 1000 * 1000  // uint baud_rate
    );
    spi_handle_t spi_handle = FatFs::add_spi(spi);

    /* Hardware Configuration of the SD Card objects */

    SdCardSpiCfg spi_sd_card(
        spi_handle,  // spi_handle_t spi_handle,
        "0:",        // const char *pcName,
        9,           // uint ss_gpio,  // Slave select for this SD card
        true,        // bool use_card_detect = false,
        13,          // uint card_detect_gpio = 0,       // Card detect; ignored if !use_card_detect
        1            // uint card_detected_true = false  // Varies with card socket; ignored if !use_card_detect
    );
    FatFs::add_sd_card(spi_sd_card);

    // Add another SD card:
    SdCardSdioCfg sdio_sd_card(
        "1:",      // const char *pcName,
        18,        // uint CMD_gpio,
        19,        // uint D0_gpio,  // D0
        true,      // bool use_card_detect = false,
        16,        // uint card_detect_gpio = 0,    // Card detect, ignored if !use_card_detect
        1,         // uint card_detected_true = 0,  // Varies with card socket, ignored if !use_card_detect
        pio1,      // PIO SDIO_PIO = pio0,          // either pio0 or pio1
        DMA_IRQ_1  // uint DMA_IRQ_num = DMA_IRQ_0  // DMA_IRQ_0 or DMA_IRQ_1
    );
    FatFs::add_sd_card(sdio_sd_card);

    if (!FatFs::begin())
        error("Driver initialization failed\n");

    if (FORMAT) {
        for (size_t i = 0; i < FatFs::SdCard_get_num(); ++i) {
            SdCard* SdCard_p = FatFs::SdCard_get_by_num(i);
            printf("Formatting drive %s...\n", SdCard_p->get_name());
            FRESULT fr = SdCard_p->format();
            chk_result("format", fr);
        }
    }
}

//------------------------------------------------------------------------------
static void bench(char const* logdrv) {
    File file;
    float s;
    uint32_t t;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t totalLatency;
    bool skipLatency;

    static_assert(0 == FILE_SIZE % BUF_SIZE,
                  "For accurate results, FILE_SIZE must be a multiple of BUF_SIZE.");

    // Insure 4-byte alignment.
    uint32_t buf32[(BUF_SIZE + 3) / 4] __attribute__((aligned(4)));
    uint8_t* buf = (uint8_t*)buf32;

    SdCard* SdCard_p(FatFs::SdCard_get_by_name(logdrv));
    if (!SdCard_p) {
        printf("Unknown logical drive name: %s\n", logdrv);
        for (;;) __breakpoint();
    }
    FRESULT fr = f_chdrive(logdrv);
    chk_result("f_chdrive", fr);

    switch (SdCard_p->fatfs()->fs_type) {
        case FS_EXFAT:
            printf("Type is exFAT\n");
            break;
        case FS_FAT12:
            printf("Type is FAT12\n");
            break;
        case FS_FAT16:
            printf("Type is FAT16\n");
            break;
        case FS_FAT32:
            printf("Type is FAT32\n");
            break;
    }

    printf("Card size: ");
    printf("%.2f", SdCard_p->get_num_sectors() * 512E-9);
    printf(" GB (GB = 1E9 bytes)\n");

    cidDmp(SdCard_p);

    // fr = f_mount(&SdCard_p->fatfs, SdCard_p->pcName, 1);
    SdCard_p->mount();
    chk_result("f_mount", fr);

    // open or create file - truncate existing file.
    fr = file.open("bench.dat", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    chk_result("open", fr);

    // fill buf with known data
    if (BUF_SIZE > 1) {
        for (size_t i = 0; i < (BUF_SIZE - 2); i++) {
            buf[i] = 'A' + (i % 26);
        }
        buf[BUF_SIZE - 2] = '\r';
    }
    buf[BUF_SIZE - 1] = '\n';

    printf("FILE_SIZE_MB = %lu\n", FILE_SIZE_MiB);
    printf("BUF_SIZE = %zu\n", BUF_SIZE);
    printf("Starting write test, please wait.\n\n");

    // do write test
    uint32_t n = FILE_SIZE / BUF_SIZE;
    printf("write speed and latency\n");
    printf("speed,max,min,avg\n");
    printf("KB/Sec,usec,usec,usec\n");
    for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
        // Open with FA_CREATE_ALWAYS creates a new file,
        // and if the file is existing, it will be truncated and overwritten.
        if (PRE_ALLOCATE) {
            fr = file.lseek(FILE_SIZE);
            chk_result("file.lseek", fr);
            if (file.tell() != FILE_SIZE) {
                printf("Disk full!\n");
                for (;;) __breakpoint();
            }
            fr = file.rewind();
            chk_result("file.rewind", fr);
        }
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();

            unsigned int bw;
            fr = file.write(buf, BUF_SIZE, &bw); /* Write it to the destination file */
            chk_result("file.write", fr);
            if (bw < BUF_SIZE) { /* error or disk full */
                error("write failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (skipLatency) {
                // Wait until first write to SD, not just a copy to the cache.
                // skipLatency = file.curPosition() < 512;
                skipLatency = file.tell() < 512;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        fr = file.sync();
        chk_result("file.sync", fr);

        t = millis() - t;
        s = file.size();
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        printf(",%lu\n", totalLatency / n);
    }
    printf("\nStarting read test, please wait.\n");
    printf("\nread speed and latency\n");
    printf("speed,max,min,avg\n");
    printf("KB/Sec,usec,usec,usec\n");

    // do read test
    for (uint8_t nTest = 0; nTest < READ_COUNT; nTest++) {
        fr = file.rewind();
        chk_result("file.rewind", fr);
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            buf[BUF_SIZE - 1] = 0;
            uint32_t m = micros();
            unsigned int nr;
            fr = file.read(buf, BUF_SIZE, &nr);
            chk_result("file.read", fr);
            if (nr != BUF_SIZE) {
                error("read failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (buf[BUF_SIZE - 1] != '\n') {
                error("data check error");
            }
            if (skipLatency) {
                skipLatency = false;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        s = file.size();
        t = millis() - t;
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        printf(",%lu\n", totalLatency / n);
    }
    printf("\nDone\n");
    fr = file.close();
    chk_result("file.close", fr);
    fr = SdCard_p->unmount();
    chk_result("file.unmount", fr);
}

void loop() {
    // put your main code here, to run repeatedly:
    printf("\nTesting drive 0:\n");
    bench("0:");
    printf("\nTesting drive 1:\n");
    bench("1:");
}
