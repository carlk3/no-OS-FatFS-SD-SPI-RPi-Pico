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

// Note: The model used here is one FatFS per SD card. 
// Multiple partitions on a card are not supported.

#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <stdint.h>
//
#include "hardware/gpio.h"
//
#include "ff.h"
//
#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

// "Class" representing SD Cards
typedef struct {
    const char *pcName;
    spi_t *spi;
    // Slave select is here in sd_card_t because multiple SDs can share an SPI
    uint ss_gpio;                   // Slave select for this SD card
    uint card_detect_gpio;    // Card detect
    uint card_detected_true;  // Varies with card socket
    // Following fields are used to keep track of the state of the card:
    int m_Status;                                    // Card status
    uint64_t sectors;                                // Assigned dynamically
    int card_type;                                   // Assigned dynamically
    FATFS fatfs;
    bool mounted;
} sd_card_t;

#define SD_BLOCK_DEVICE_ERROR_NONE 0
#define SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK -5001 /*!< operation would block */
#define SD_BLOCK_DEVICE_ERROR_UNSUPPORTED -5002 /*!< unsupported operation */
#define SD_BLOCK_DEVICE_ERROR_PARAMETER -5003   /*!< invalid parameter */
#define SD_BLOCK_DEVICE_ERROR_NO_INIT -5004     /*!< uninitialized */
#define SD_BLOCK_DEVICE_ERROR_NO_DEVICE \
    -5005 /*!< device is missing or not connected */
#define SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED -5006 /*!< write protected */
#define SD_BLOCK_DEVICE_ERROR_UNUSABLE -5007        /*!< unusable card */
#define SD_BLOCK_DEVICE_ERROR_NO_RESPONSE                              \
    -5008                                 /*!< No response from device \
                                           */
#define SD_BLOCK_DEVICE_ERROR_CRC -5009   /*!< CRC error */
#define SD_BLOCK_DEVICE_ERROR_ERASE -5010 /*!< Erase error: reset/sequence */
#define SD_BLOCK_DEVICE_ERROR_WRITE \
    -5011 /*!< SPI Write error: !SPI_DATA_ACCEPTED */

///* Disk Status Bits (DSTATUS) */
// See diskio.h.
//enum {
//    STA_NOINIT = 0x01, /* Drive not initialized */
//    STA_NODISK = 0x02, /* No medium in the drive */
//    STA_PROTECT = 0x04 /* Write protected */
//};

bool sd_init_driver();
int sd_init_card(sd_card_t *pSD);
int sd_write_blocks(sd_card_t *pSD, const uint8_t *buffer,
                    uint64_t ulSectorNumber, uint32_t blockCnt);
int sd_read_blocks(sd_card_t *pSD, uint8_t *buffer, uint64_t ulSectorNumber,
                   uint32_t ulSectorCount);
bool sd_card_detect(sd_card_t *pSD);
uint64_t sd_sectors(sd_card_t *pSD);

#ifdef __cplusplus
}
#endif

#endif
/* [] END OF FILE */
