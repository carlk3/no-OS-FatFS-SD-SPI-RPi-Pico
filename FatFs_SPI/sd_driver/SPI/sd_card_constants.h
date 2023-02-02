/* sd_card_constants.h
Copyright 2021 Carl John Kugler III

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


// Only HC block size is supported. Making this a static constant reduces code
// size.
#define BLOCK_SIZE_HC 512 /*!< Block size supported for SD card is 512 bytes */
static const uint32_t _block_size = BLOCK_SIZE_HC;

#define SD_BLOCK_DEVICE_ERROR_NONE 0
#define SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK -5001 /*!< operation would block */
#define SD_BLOCK_DEVICE_ERROR_UNSUPPORTED -5002 /*!< unsupported operation */
#define SD_BLOCK_DEVICE_ERROR_PARAMETER -5003   /*!< invalid parameter */
#define SD_BLOCK_DEVICE_ERROR_NO_INIT -5004     /*!< uninitialized */
#define SD_BLOCK_DEVICE_ERROR_NO_DEVICE -5005   /*!< device is missing or not connected */
#define SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED -5006 /*!< write protected */
#define SD_BLOCK_DEVICE_ERROR_UNUSABLE -5007    /*!< unusable card */
#define SD_BLOCK_DEVICE_ERROR_NO_RESPONSE -5008 /*!< No response from device */
#define SD_BLOCK_DEVICE_ERROR_CRC -5009    /*!< CRC error */
#define SD_BLOCK_DEVICE_ERROR_ERASE -5010 /*!< Erase error: reset/sequence */
#define SD_BLOCK_DEVICE_ERROR_WRITE -5011 /*!< SPI Write error: !SPI_DATA_ACCEPTED */

/** Represents the different SD/MMC card types  */
// Types
#define SDCARD_NONE 0  /**< No card is present */
#define SDCARD_V1 1    /**< v1.x Standard Capacity */
#define SDCARD_V2 2    /**< v2.x Standard capacity SD card */
#define SDCARD_V2HC 3  /**< v2.x High capacity SD card */
#define CARD_UNKNOWN 4 /**< Unknown or unsupported card */

// Supported SD Card Commands
typedef enum {
    CMD_NOT_SUPPORTED = -1,             /**< Command not supported error */
    CMD0_GO_IDLE_STATE = 0,             /**< Resets the SD Memory Card */
    CMD1_SEND_OP_COND = 1,              /**< Sends host capacity support */
    CMD6_SWITCH_FUNC = 6,               /**< Check and Switches card function */
    CMD8_SEND_IF_COND = 8,              /**< Supply voltage info */
    CMD9_SEND_CSD = 9,                  /**< Provides Card Specific data */
    CMD10_SEND_CID = 10,                /**< Provides Card Identification */
    CMD12_STOP_TRANSMISSION = 12,       /**< Forces the card to stop transmission */
    CMD13_SEND_STATUS = 13,             /**< Card responds with status */
    CMD16_SET_BLOCKLEN = 16,            /**< Length for SC card is set */
    CMD17_READ_SINGLE_BLOCK = 17,       /**< Read single block of data */
    CMD18_READ_MULTIPLE_BLOCK = 18,     /**< Card transfers data blocks to host
         until interrupted by a STOP_TRANSMISSION command */
    CMD24_WRITE_BLOCK = 24,             /**< Write single block of data */
    CMD25_WRITE_MULTIPLE_BLOCK = 25,    /**< Continuously writes blocks of data
        until    'Stop Tran' token is sent */
    CMD27_PROGRAM_CSD = 27,             /**< Programming bits of CSD */
    CMD32_ERASE_WR_BLK_START_ADDR = 32, /**< Sets the address of the first write
     block to be erased. */
    CMD33_ERASE_WR_BLK_END_ADDR = 33,   /**< Sets the address of the last write
       block of the continuous range to be erased.*/
    CMD38_ERASE = 38,                   /**< Erases all previously selected write blocks */
    CMD55_APP_CMD = 55,                 /**< Extend to Applications specific commands */
    CMD56_GEN_CMD = 56,                 /**< General Purpose Command */
    CMD58_READ_OCR = 58,                /**< Read OCR register of card */
    CMD59_CRC_ON_OFF = 59,              /**< Turns the CRC option on or off*/
    // App Commands
    ACMD6_SET_BUS_WIDTH = 6,
    ACMD13_SD_STATUS = 13,
    ACMD22_SEND_NUM_WR_BLOCKS = 22,
    ACMD23_SET_WR_BLK_ERASE_COUNT = 23,
    ACMD41_SD_SEND_OP_COND = 41,
    ACMD42_SET_CLR_CARD_DETECT = 42,
    ACMD51_SEND_SCR = 51,
} cmdSupported;

///* Disk Status Bits (DSTATUS) */
// See diskio.h.
//enum {
//    STA_NOINIT = 0x01, /* Drive not initialized */
//    STA_NODISK = 0x02, /* No medium in the drive */
//    STA_PROTECT = 0x04 /* Write protected */
//};

