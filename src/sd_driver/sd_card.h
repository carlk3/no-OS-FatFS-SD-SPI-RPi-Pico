/* sd_card.h
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

// Note: The model used here is one FatFS per SD card. 
// Multiple partitions on a card are not supported.

#pragma once

#include <stdint.h>
//
#include "hardware/gpio.h"
#include <hardware/pio.h>
#include "pico/mutex.h"
//
#include "ff.h"
//
#include "SPI/spi.h"
#include "SPI/sd_card_constants.h"
//
#include "SdCardInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_IF_SPI,
    SD_IF_SDIO
} sd_if_t;

typedef struct sd_spi_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs.
    // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
    // GPIO_DRIVE_STRENGTH_12MA = 3 }
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_t;

typedef struct sd_sdio_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1

    /* The following fields are not part of the configuration. They are dynamically assigned. */
    int SDIO_DMA_CH;
    int SDIO_DMA_CHB;
    int SDIO_CMD_SM;
    int SDIO_DATA_SM;
} sd_sdio_t;

typedef struct sd_card_t sd_card_t;

// "Class" representing SD Cards
struct sd_card_t {
    const char *pcName;
    sd_if_t type;
    union {
        sd_spi_t spi_if;
        sd_sdio_t sdio_if;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect

    /* The following fields are not part of the configuration. They are dynamically assigned. */
    int m_Status;                                    // Card status
    uint64_t sectors;                                // Assigned dynamically
    int card_type;                                   // Assigned dynamically
    mutex_t mutex;
    FATFS fatfs;
    bool mounted;

    int (*init)(sd_card_t *sd_card_p);
    int (*write_blocks)(sd_card_t *sd_card_p, const uint8_t *buffer,
                    uint64_t ulSectorNumber, uint32_t blockCnt);
    int (*read_blocks)(sd_card_t *sd_card_p, uint8_t *buffer, uint64_t ulSectorNumber,
                    uint32_t ulSectorCount);    
    uint64_t (*get_num_sectors)(sd_card_t *sd_card_p);
    bool (*sd_readCID)(sd_card_t *sd_card_p, cid_t *cid);
};
bool sd_init_driver();
bool sd_card_detect(sd_card_t *sd_card_p);

#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
