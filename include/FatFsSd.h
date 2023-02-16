/* Sd.h
Copyright 2023 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

// See FatFs - Generic FAT Filesystem Module, "Application Interface",
// http://elm-chan.org/fsw/ff/00index_e.html

#pragma once

#include <cstring>
#include <vector>

#include "FatFsSd_C.h"

namespace FatFsNs {

class Spi {
   protected:
    spi_t m_spi = {};

   public:
    friend class FatFs;
    friend class SdCardSpiCfg;
    friend spi_t* ::spi_get_by_num(size_t num);
};
class SpiCfg : public Spi {
   public:
    SpiCfg(
        spi_inst_t* hw_inst,
        uint miso_gpio,                     // SPI MISO GPIO number
        uint mosi_gpio,                     // SPI MOSI GPIO number
        uint sck_gpio,                      // SPI SCLK GPIO number
        uint baud_rate = 25 * 1000 * 1000,  // Frequency of the SPI clock
        uint DMA_IRQ_num = DMA_IRQ_0,       // DMA_IRQ_0 or DMA_IRQ_1
        bool set_drive_strength = false,    // Whether or not to set GPIO drive strength
        enum gpio_drive_strength mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        enum gpio_drive_strength sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA) {
        m_spi = {
            .hw_inst = hw_inst,
            .miso_gpio = miso_gpio,  // SPI MISO GPIO number (not pin number)
            .mosi_gpio = mosi_gpio,
            .sck_gpio = sck_gpio,
            .baud_rate = baud_rate,
            .DMA_IRQ_num = DMA_IRQ_num,
            .set_drive_strength = set_drive_strength,
            .mosi_gpio_drive_strength = mosi_gpio_drive_strength,  // DMA_IRQ_0 or DMA_IRQ_1
            .sck_gpio_drive_strength = sck_gpio_drive_strength};
    }
};

typedef Spi* spi_handle_t;

class SdCard {
   protected:
    sd_card_t m_sd_card = {};
    SdCard() {}

   public:
    SdCard(const SdCard&) = default;

    const char* get_name() { return m_sd_card.pcName; }

    FRESULT mount() {
        return f_mount(&m_sd_card.fatfs, m_sd_card.pcName, 1);
    }
    FRESULT unmount() {
        return f_unmount(m_sd_card.pcName);
    }
    static FRESULT getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs) { /* Get number of free clusters on the drive */
        return f_getfree(path, nclst, fatfs);
    }
    static FRESULT getlabel(const TCHAR* path, TCHAR* label, DWORD* vsn) { /* Get volume label */
        return f_getlabel(path, label, vsn);
    }
    static FRESULT setlabel(const TCHAR* label) { /* Set volume label */
        return f_setlabel(label);
    }
    static FRESULT mkfs(const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len) { /* Create a FAT volume */
        return f_mkfs(path, opt, work, len);
    }
    /* Create a FAT volume: format with defaults */
    FRESULT format() {
        const char* name = get_name();
        return mkfs(name, 0, 0, FF_MAX_SS * 2);
    }
    static FRESULT fdisk(BYTE pdrv, const LBA_t ptbl[], void* work) { /* Divide a physical drive into some partitions */
        return f_fdisk(pdrv, ptbl, work);
    }
    bool readCID(cid_t* cid) {
        return m_sd_card.sd_readCID(&m_sd_card, cid);
    }
    FATFS* fatfs() {
        return &m_sd_card.fatfs;
    }
    uint64_t get_num_sectors() {
        return m_sd_card.get_num_sectors(&m_sd_card);
    }
    friend sd_card_t* ::sd_get_by_num(size_t num);
};

class SdCardSpiCfg : public SdCard {
   public:
    SdCardSpiCfg(
        spi_handle_t Spi_p,               // Pointer to the spi_t instance that drives this card
        const char* pcName,               // Name used to mount device
        uint ss_gpio,                     // Slave select for this SD card
        bool use_card_detect = false,     // Whether or not to use Card Detect
        uint card_detect_gpio = 0,        // Card detect; ignored if !use_card_detect
        uint card_detected_true = false,  // Varies with card socket; ignored if !use_card_detect
        bool set_drive_strength = false,  // Whether or not to set the SPIO drive strength
        enum gpio_drive_strength ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA)
    // : SdCard(pcName)
    {
        m_sd_card = {
            .pcName = pcName,
            .type = SD_IF_SPI,
            .spi_if = {
                .spi = &Spi_p->m_spi,  // Pointer to the SPI driving this card
                .ss_gpio = ss_gpio,    // The SPI slave select GPIO for this SD card
                .set_drive_strength = set_drive_strength,
                .ss_gpio_drive_strength = ss_gpio_drive_strength},
            .use_card_detect = use_card_detect,
            .card_detect_gpio = card_detect_gpio,     // Card detect
            .card_detected_true = card_detected_true  // What the GPIO read returns when a card is present.
        };
    }
};
class SdCardSdioCfg : public SdCard {
   public:
    SdCardSdioCfg(
        const char* pcName,  // Name used to mount device
        uint CMD_gpio,
        uint D0_gpio,  // D0
        bool use_card_detect = false,
        uint card_detect_gpio = 0,    // Card detect, ignored if !use_card_detect
        uint card_detected_true = 0,  // Varies with card socket, ignored if !use_card_detect
        PIO SDIO_PIO = pio0,          // either pio0 or pio1
        uint DMA_IRQ_num = DMA_IRQ_0  // DMA_IRQ_0 or DMA_IRQ_1
        )
    // : SdCard(pcName)
    {
        m_sd_card = {
            .pcName = pcName,
            .type = SD_IF_SDIO,
            .sdio_if = {
                .CMD_gpio = CMD_gpio,
                .D0_gpio = D0_gpio,
                .SDIO_PIO = SDIO_PIO,
                .DMA_IRQ_num = DMA_IRQ_num},
            .use_card_detect = use_card_detect,
            .card_detect_gpio = card_detect_gpio,     // Card detect
            .card_detected_true = card_detected_true  // What the GPIO read returns when a card is present.
        };
    }
};

class FatFs {
    static std::vector<Spi> Spis;
    static std::vector<SdCard> SdCards;

   public:
    static spi_handle_t add_spi(SpiCfg& Spi) {
        Spis.push_back(Spi);
        return &Spis.back();
    }
    static SdCard* add_sd_card(SdCard& SdCard) {
        SdCards.push_back(SdCard);
        return &SdCards.back();
    }

    static FRESULT chdrive(const TCHAR* path) {
        return f_chdrive(path);
    }
    static FRESULT setcp(WORD cp) { /* Set current code page */
        return f_setcp(cp);
    }
    static bool begin();

    static size_t SdCard_get_num() {
        return SdCards.size();
    }
    static SdCard* SdCard_get_by_num(size_t num) {
        if (num <= SdCard_get_num()) {
            return &SdCards[num];
        } else {
            return NULL;
        }
    }
    static SdCard* SdCard_get_by_name(const char* const name) {
        for (size_t i = 0; i < SdCard_get_num(); ++i)
            if (0 == strcmp(SdCard_get_by_num(i)->get_name(), name))
                return SdCard_get_by_num(i);
        // printf("%s: unknown name %s\n", __func__, name);
        return NULL;
    }
    static size_t Spi_get_num() {
        return Spis.size();
    }
    static Spi* Spi_get_by_num(size_t num) {
        if (num <= Spi_get_num()) {
            return &Spis[num];
        } else {
            return NULL;
        }
    }
};

class File {
    FIL fil;

   public:
    FRESULT open(const TCHAR* path, BYTE mode) { /* Open or create a file */
        return f_open(&fil, path, mode);
    }
    FRESULT close() { /* Close an open file object */
        return f_close(&fil);
    }
    FRESULT read(void* buff, UINT btr, UINT* br) { /* Read data from the file */
        return f_read(&fil, buff, btr, br);
    }
    FRESULT write(const void* buff, UINT btw, UINT* bw) { /* Write data to the file */
        return f_write(&fil, buff, btw, bw);
    }
    FRESULT lseek(FSIZE_t ofs) { /* Move file pointer of the file object */
        return f_lseek(&fil, ofs);
    }
    FRESULT truncate() { /* Truncate the file */
        return f_truncate(&fil);
    }
    FRESULT sync() { /* Flush cached data of the writing file */
        return f_sync(&fil);
    }
    int putc(TCHAR c) { /* Put a character to the file */
        return f_putc(c, &fil);
    }
    int puts(const TCHAR* str) { /* Put a string to the file */
        return f_puts(str, &fil);
    }
    int printf(const TCHAR* str, ...);  /* Put a formatted string to the file. Returns -1 on error. */
    TCHAR* gets(TCHAR* buff, int len) { /* Get a string from the file */
        return f_gets(buff, len, &fil);
    }
    bool eof() {
        return f_eof(&fil);
    }
    BYTE error() {
        return f_error(&fil);
    }
    FSIZE_t tell() {
        return f_tell(&fil);
    }
    FSIZE_t size() {
        return f_size(&fil);
    }
    FRESULT rewind() {
        return f_rewind(&fil);
    }
    FRESULT forward(UINT (*func)(const BYTE*, UINT), UINT btf, UINT* bf) { /* Forward data to the stream */
        return f_forward(&fil, func, btf, bf);
    }
    FRESULT expand(FSIZE_t fsz, BYTE opt) { /* Allocate a contiguous block to the file */
        return f_expand(&fil, fsz, opt);
    }
};

class Dir {
    DIR dir = {};

   public:
    FRESULT rewinddir() {
        return f_rewinddir(&dir);
    }
    FRESULT rmdir(const TCHAR* path) {
        return f_rmdir(path);
    }
    FRESULT opendir(const TCHAR* path) { /* Open a directory */
        return f_opendir(&dir, path);
    }
    FRESULT closedir() { /* Close an open directory */
        return f_closedir(&dir);
    }
    FRESULT readdir(FILINFO* fno) { /* Read a directory item */
        return f_readdir(&dir, fno);
    }
    FRESULT findfirst(FILINFO* fno, const TCHAR* path, const TCHAR* pattern) { /* Find first file */
        return f_findfirst(&dir, fno, path, pattern);
    }
    FRESULT findnext(FILINFO* fno) { /* Find next file */
        return f_findnext(&dir, fno);
    }
    static FRESULT mkdir(const TCHAR* path) { /* Create a sub directory */
        return f_mkdir(path);
    }
    static FRESULT unlink(const TCHAR* path) { /* Delete an existing file or directory */
        return f_unlink(path);
    }
    static FRESULT rename(const TCHAR* path_old, const TCHAR* path_new) { /* Rename/Move a file or directory */
        return f_rename(path_old, path_new);
    }
    static FRESULT stat(const TCHAR* path, FILINFO* fno) { /* Get file status */
        return f_stat(path, fno);
    }
    static FRESULT chmod(const TCHAR* path, BYTE attr, BYTE mask) { /* Change attribute of a file/dir */
        return f_chmod(path, attr, mask);
    }
    static FRESULT utime(const TCHAR* path, const FILINFO* fno) { /* Change timestamp of a file/dir */
        return f_utime(path, fno);
    }
    static FRESULT chdir(const TCHAR* path) { /* Change current directory */
        return f_chdir(path);
    }
    static FRESULT chdrive(const TCHAR* path) { /* Change current drive */
        return f_chdrive(path);
    }
    static FRESULT getcwd(TCHAR* buff, UINT len) { /* Get current directory */
        return f_getcwd(buff, len);
    }
};
}  // namespace FatFsNs
