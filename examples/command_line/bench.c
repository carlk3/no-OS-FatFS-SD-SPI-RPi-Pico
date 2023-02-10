/* Ported from: https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino
 *
 * This program is a simple binary write/read benchmark.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "SdCardInfo.h"
#include "SDIO/SdioCard.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#define error(s)                  \
    {                             \
        printf("ERROR: %s\n", s); \
        assert(!s);               \
    }

static uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}
static uint64_t micros() {
    return to_us_since_boot(get_absolute_time());
}
static sd_card_t* sd_get_by_name(const char* const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
    printf("%s: unknown name %s\n", __func__, name);
    return NULL;
}

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

static FIL file;
static sd_card_t* sd_card_p;

static void cidDmp() {
    cid_t cid;
    if (!sd_card_p->sd_readCID(sd_card_p, &cid)) {
        error("readCID failed");
    }
    printf("\nManufacturer ID: ");
    // cout << uppercase << showbase << hex << int(cid.mid) << dec << endl;
    printf("0x%x\n", cid.mid);
    printf("OEM ID: ");
    // << cid.oid[0] << cid.oid[1] << endl;
    printf("%c%c\n", cid.oid[0], cid.oid[1]);
    printf("Product: ");
    for (uint8_t i = 0; i < 5; i++) {
        printf("%c", cid.pnm[i]);
    }
    printf("\nRevision: ");
    //  << cid.prvN() << '.' << cid.prvM() << endl;
    // int CID_prvN(cid_t *cid_p)
    // int CID_prvM(cid_t *cid_p)
    printf("%d.%d\n", CID_prvN(&cid), CID_prvM(&cid));
    printf("Serial number: ");
    //  << hex << cid.psn() << dec << endl;
    // uint32_t CID_psn(cid_t *cid_p)
    printf("0x%lx\n", CID_psn(&cid));
    printf("Manufacturing date: ");
    // cout << cid.mdtMonth() << '/' << cid.mdtYear() << endl;
    // int CID_mdtMonth(cid_t *cid_p)
    // int CID_mdtYear(cid_t *cid_p)
    printf("%d/%d\n", CID_mdtMonth(&cid), CID_mdtYear(&cid));
    // cout << endl;
    printf("\n");
}
//------------------------------------------------------------------------------
void bench(char const* logdrv) {
    float s;
    uint32_t t;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t totalLatency;
    bool skipLatency;    

    static_assert(0 == FILE_SIZE % BUF_SIZE, 
            "For accurate results, FILE_SIZE must be a multiple of BUF_SIZE.");

    // Insure 4-byte alignment.
    uint32_t buf32[(BUF_SIZE + 3) / 4] __attribute__ ((aligned (4)));
    uint8_t* buf = (uint8_t*)buf32;

    sd_card_p = sd_get_by_name(logdrv);
    if (!sd_card_p) {
        printf("Unknown logical drive name: %s\n", logdrv);
        return;
    }
    FRESULT fr = f_chdrive(logdrv);
    if (FR_OK != fr) {
        printf("f_chdrive error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    // if (sd.fatType() == FAT_TYPE_EXFAT) {
    //     printf("Type is exFAT\n");
    // } else {
    //     printf("Type is FAT") << int(sd.fatType()) << endl;
    // }
    switch (sd_card_p->fatfs.fs_type) {
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
    // << sd.card()->sectorCount() * 512E-9;
    printf("%.2f", sd_card_p->get_num_sectors(sd_card_p) * 512E-9);
    printf(" GB (GB = 1E9 bytes)\n");

    cidDmp();

    // open or create file - truncate existing file.
    // if (!file.open("bench.dat", O_RDWR | O_CREAT | O_TRUNC)) {
    //     error("open failed");
    // }
    fr = f_open(&file, "bench.dat", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    // fill buf with known data
    if (BUF_SIZE > 1) {
        for (size_t i = 0; i < (BUF_SIZE - 2); i++) {
            buf[i] = 'A' + (i % 26);
        }
        buf[BUF_SIZE - 2] = '\r';
    }
    buf[BUF_SIZE - 1] = '\n';

    printf("FILE_SIZE_MB = %lu\n", FILE_SIZE_MiB);     // << FILE_SIZE_MB << endl;
    printf("BUF_SIZE = %zu\n", BUF_SIZE);             // << BUF_SIZE << F(" bytes\n");
    printf("Starting write test, please wait.\n\n");  // << endl
                                                      // << endl;
    // do write test
    uint32_t n = FILE_SIZE / BUF_SIZE;
    printf("write speed and latency\n");
    printf("speed,max,min,avg\n");
    printf("KB/Sec,usec,usec,usec\n");
    for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
        // file.truncate(0);
        // Open with FA_CREATE_ALWAYS creates a new file,
        // and if the file is existing, it will be truncated and overwritten.
        if (PRE_ALLOCATE) {
            // if (!file.preAllocate(FILE_SIZE)) {
            //     error("preAllocate failed");
            // }
            fr = f_lseek(&file, FILE_SIZE);
            if (FR_OK != fr) {
                printf("f_lseek error: %s (%d)\n", FRESULT_str(fr), fr);
                return;
            }
            if (f_tell(&file) != FILE_SIZE) {
                printf("Disk full!\n");
                return;
            }
            fr = f_rewind(&file);
            if (FR_OK != fr) {
                printf("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
                return;
            }
        }
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();

            // if (file.write(buf, BUF_SIZE) != BUF_SIZE) {
            //     error("write failed");
            // }
            unsigned int bw;
            fr = f_write(&file, buf, BUF_SIZE, &bw); /* Write it to the destination file */
            if (FR_OK != fr) {
                printf("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
                return;
            }
            if (bw < BUF_SIZE) { /* error or disk full */
                error("write failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (skipLatency) {
                // Wait until first write to SD, not just a copy to the cache.
                // skipLatency = file.curPosition() < 512;
                skipLatency = f_tell(&file) < 512;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        // file.sync();
        fr = f_sync(&file);
        if (FR_OK != fr) {
            printf("f_sync error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        t = millis() - t;
        // s = file.fileSize();
        s = f_size(&file);
        // cout << s / t << ',' << maxLatency << ',' << minLatency;
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        // cout << ',' << totalLatency / n << endl;
        printf(",%lu\n", totalLatency / n);
    }
    printf("\nStarting read test, please wait.\n");
    printf("\nread speed and latency\n");
    printf("speed,max,min,avg\n");
    printf("KB/Sec,usec,usec,usec\n");

    // do read test
    for (uint8_t nTest = 0; nTest < READ_COUNT; nTest++) {
        // file.rewind();
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            printf("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            buf[BUF_SIZE - 1] = 0;
            uint32_t m = micros();
            // int32_t nr = file.read(buf, BUF_SIZE);
            unsigned int nr;
            fr = f_read(&file, buf, BUF_SIZE, &nr);
            if (FR_OK != fr) {
                printf("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
                return;
            }
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
        // s = file.fileSize();
        s = f_size(&file);
        t = millis() - t;
        // cout << s / t << ',' << maxLatency << ',' << minLatency;
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        // cout << ',' << totalLatency / n << endl;
        printf(",%lu\n", totalLatency / n);
    }
    printf("\nDone\n");
    // file.close();
    fr = f_close(&file);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    // sd.end();
}
