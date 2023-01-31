/* sd_card.c
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

/* Standard includes. */
#include <inttypes.h>
#include <string.h>
//
#include "pico/mutex.h"
//
#include "hw_config.h"  // Hardware Configuration of the SPI and SD Card "objects"
#include "my_debug.h"
#include "SPI/sd_card_spi.h"
#include "SDIO/SdioCard.h"
//
#include "sd_card.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */  // Needed for STA_NOINIT, ...

#define TRACE_PRINTF(fmt, args...)
// #define TRACE_PRINTF printf

/* Return non-zero if the SD-card is present. */
bool sd_card_detect(sd_card_t *pSD) {
    TRACE_PRINTF("> %s\r\n", __FUNCTION__);
    if (!pSD->use_card_detect) {
        pSD->m_Status &= ~STA_NODISK;
        return true;
    }
    /*!< Check GPIO to detect SD */
    if (gpio_get(pSD->card_detect_gpio) == pSD->card_detected_true) {
        // The socket is now occupied
        pSD->m_Status &= ~STA_NODISK;
        TRACE_PRINTF("SD card detected!\r\n");
        return true;
    } else {
        // The socket is now empty
        pSD->m_Status |= (STA_NODISK | STA_NOINIT);
        pSD->card_type = SDCARD_NONE;
        printf("No SD card detected!\r\n");
        return false;
    }
}

bool sd_init_driver() {
    static bool initialized;
    auto_init_mutex(initialized_mutex);
    mutex_enter_blocking(&initialized_mutex);
    if (!initialized) {
        for (size_t i = 0; i < sd_get_num(); ++i) {
            sd_card_t *pSD = sd_get_by_num(i);
            switch (pSD->type) {
                case SD_IF_SPI:
                    sd_spi_ctor(pSD);
                    break;
                case SD_IF_SDIO:
                    sd_sdio_ctor(pSD);
                    break;
            }  // switch (pSD->type)
        }      // for
        for (size_t i = 0; i < spi_get_num(); ++i) {
            spi_t *pSPI = spi_get_by_num(i);
            if (!my_spi_init(pSPI)) {
                mutex_exit(&initialized_mutex);
                return false;
            }
        }
        initialized = true;
    }
    mutex_exit(&initialized_mutex);
    return true;
}
/* [] END OF FILE */
