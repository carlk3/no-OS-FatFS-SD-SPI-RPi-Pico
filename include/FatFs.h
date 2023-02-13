/* FatFs_Sd.h
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
#pragma once

#include <string.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "FatFs_C.h"

struct FatFs_SdException : public std::runtime_error {
    FatFs_SdException(const std::string &msg) : std::runtime_error{msg} {}
};
struct FatFs_CallException : public FatFs_SdException {
    FatFs_CallException(const std::string &msg, FRESULT fr)
        : FatFs_SdException{msg + std::string(FRESULT_str(fr)) + "(" + std::to_string(fr) + ")"} {}
};

class FatFs_Spi {
    spi_t m_spi = {};
    friend class FatFs_SdCardSpi;
    friend spi_t *spi_get_by_num(size_t num);

   public:
    FatFs_Spi(
        spi_inst_t *hw_inst,
        uint miso_gpio,  // SPI MISO GPIO number (not pin number)
        uint mosi_gpio,
        uint sck_gpio,
        uint baud_rate = 25 * 1000 * 1000,
        uint DMA_IRQ_num = DMA_IRQ_0,  // DMA_IRQ_0 or DMA_IRQ_1
        bool set_drive_strength = false,
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

class FatFs_SdCard {
   protected:
    std::string m_name;
    sd_card_t m_sd_card = {};

    FatFs_SdCard(const char *name) : m_name(name) {}

   public:
    const char *get_name() { return m_sd_card.pcName; }
    FRESULT mount();
    FRESULT unmount();

    friend sd_card_t *sd_get_by_num(size_t num);

    virtual ~FatFs_SdCard() = 0;
};

class FatFs_SdCardSpi : public FatFs_SdCard {
   public:
    FatFs_SdCardSpi(
        FatFs_Spi &ffs,
        const char *pcName,  // Name used to mount device
        uint ss_gpio,        // Slave select for this SD card
        bool use_card_detect = false,
        uint card_detect_gpio = 0,        // Card detect; ignored if !use_card_detect
        uint card_detected_true = false,  // Varies with card socket; ignored if !use_card_detect
        bool set_drive_strength = false,
        enum gpio_drive_strength ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA)
        : FatFs_SdCard(pcName) {
        m_sd_card = {
            .pcName = m_name.c_str(),
            .type = SD_IF_SPI,
            .spi_if = {
                .spi = &ffs.m_spi,   // Pointer to the SPI driving this card
                .ss_gpio = ss_gpio,  // The SPI slave select GPIO for this SD card
                .set_drive_strength = set_drive_strength,
                .ss_gpio_drive_strength = ss_gpio_drive_strength},
            .use_card_detect = use_card_detect,
            .card_detect_gpio = card_detect_gpio,     // Card detect
            .card_detected_true = card_detected_true  // What the GPIO read returns when a card is present.
        };
    }
};
class FatFs_SdCardSdio : public FatFs_SdCard {
   public:
    FatFs_SdCardSdio(
        const char *pcName,  // Name used to mount device
        uint CMD_gpio,
        uint D0_gpio,  // D0
        bool use_card_detect = false,
        uint card_detect_gpio = 0,    // Card detect, ignored if !use_card_detect
        uint card_detected_true = 0,  // Varies with card socket, ignored if !use_card_detect
        PIO SDIO_PIO = pio0,          // either pio0 or pio1
        uint DMA_IRQ_num = DMA_IRQ_0  // DMA_IRQ_0 or DMA_IRQ_1
    ) 
    : FatFs_SdCard(pcName) {
        m_sd_card = {
            .pcName = m_name.c_str(),
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
    static std::vector<FatFs_Spi *> Spi_ps;
    static std::vector<FatFs_SdCard *> SdCard_ps;

   public:
    static void add_spi_p(FatFs_Spi *Spi_p) { Spi_ps.push_back(Spi_p); }
    static void add_sd_card_p(FatFs_SdCard *SdCard_p) { SdCard_ps.push_back(SdCard_p); }
    static size_t SdCard_get_num() { return SdCard_ps.size(); }
    static FatFs_SdCard *SdCard_get_by_num(size_t num) {
        if (num <= SdCard_get_num()) {
            return SdCard_ps[num];
        } else {
            return NULL;
        }
    }
    static size_t Spi_get_num() { return Spi_ps.size(); }
    static FatFs_Spi *Spi_get_by_num(size_t num) {
        if (num <= Spi_get_num()) {
            return Spi_ps[num];
        } else {
            return NULL;
        }
    }
    static FRESULT chdrive(const TCHAR *path);
};

class FatFs_File {
    FIL fil;

   public:
    FRESULT open(const TCHAR *path, BYTE mode);          /* Open or create a file */
    FRESULT close();                                     /* Close an open file object */
    FRESULT read(void *buff, UINT btr, UINT *br);        /* Read data from the file */
    FRESULT write(const void *buff, UINT btw, UINT *bw); /* Write data to the file */
    FRESULT lseek(FSIZE_t ofs);                          /* Move file pointer of the file object */
    FRESULT truncate();                                  /* Truncate the file */
    FRESULT sync();                                      /* Flush cached data of the writing file */
    int putc(TCHAR c);                                   /* Put a character to the file */
    int puts(const TCHAR *str);                          /* Put a string to the file */
    int printf(const TCHAR *str, ...);                   /* Put a formatted string to the file */
    TCHAR *gets(TCHAR *buff, int len);                   /* Get a string from the file */
    bool eof();
    BYTE error();
    FSIZE_t tell();
    FSIZE_t size();
    int rewind();
};

// #define f_rewinddir(dp) f_readdir((dp), 0)
// #define f_rmdir(path) f_unlink(path)

// FRESULT opendir (DIR* dp, const TCHAR* path);						/* Open a directory */
// FRESULT closedir (DIR* dp);										/* Close an open directory */
// FRESULT readdir (DIR* dp, FILINFO* fno);							/* Read a directory item */
// FRESULT findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);	/* Find first file */
// FRESULT findnext (DIR* dp, FILINFO* fno);							/* Find next file */
// FRESULT mkdir (const TCHAR* path);								/* Create a sub directory */
// FRESULT unlink (const TCHAR* path);								/* Delete an existing file or directory */
// FRESULT rename (const TCHAR* path_old, const TCHAR* path_new);	/* Rename/Move a file or directory */
// FRESULT stat (const TCHAR* path, FILINFO* fno);					/* Get file status */
// FRESULT chmod (const TCHAR* path, BYTE attr, BYTE mask);			/* Change attribute of a file/dir */
// FRESULT utime (const TCHAR* path, const FILINFO* fno);			/* Change timestamp of a file/dir */
// FRESULT chdir (const TCHAR* path);								/* Change current directory */
// FRESULT chdrive (const TCHAR* path);								/* Change current drive */
// FRESULT getcwd (TCHAR* buff, UINT len);							/* Get current directory */
// FRESULT getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);	/* Get number of free clusters on the drive */
// FRESULT getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);	/* Get volume label */
// FRESULT setlabel (const TCHAR* label);							/* Set volume label */
// FRESULT forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf);	/* Forward data to the stream */
// FRESULT expand (FIL* fp, FSIZE_t fsz, BYTE opt);					/* Allocate a contiguous block to the file */
// FRESULT mount (FATFS* fs, const TCHAR* path, BYTE opt);			/* Mount/Unmount a logical drive */
// FRESULT mkfs (const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len);	/* Create a FAT volume */
// FRESULT fdisk (BYTE pdrv, const LBA_t ptbl[], void* work);		/* Divide a physical drive into some partitions */
// FRESULT setcp (WORD cp);											/* Set current code page */
