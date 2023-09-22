/* sd_spi.h
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

#ifndef _SD_SPI_H_
#define _SD_SPI_H_

#include <stdint.h>
#include "sd_card.h"

/* Transfer tx to SPI while receiving SPI to rx. 
tx or rx can be NULL if not important. */
void sd_spi_go_low_frequency(sd_card_t *this);
void sd_spi_go_high_frequency(sd_card_t *this);
void sd_spi_deselect_pulse(sd_card_t *sd_card_p);

/* 
After power up, the host starts the clock and sends the initializing sequence on the CMD line. 
This sequence is a contiguous stream of logical ‘1’s. The sequence length is the maximum of 1msec, 
74 clocks or the supply-ramp-uptime; the additional 10 clocks 
(over the 64 clocks after what the card should be ready for communication) is
provided to eliminate power-up synchronization problems. 
*/
void sd_spi_send_initializing_sequence(sd_card_t * sd_card_p);

static inline uint8_t sd_spi_write(sd_card_t *sd_card_p, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    uint8_t received = SPI_FILL_CHAR;
#if 1
    int num = spi_write_read_blocking(sd_card_p->spi_if.spi->hw_inst, &value, &received, 1);    
    assert(1 == num);
#else
    bool success = spi_transfer(sd_card_p->spi_if.spi, &value, &received, 1);
    assert(success);
#endif
    return received;
}

// Would do nothing if sd_card_p->spi_if.ss_gpio were set to GPIO_FUNC_SPI.
static inline void sd_spi_select(sd_card_t *sd_card_p) {
    gpio_put(sd_card_p->spi_if.ss_gpio, 0);
    // See http://elm-chan.org/docs/mmc/mmc_e.html#spibus
    sd_spi_write(sd_card_p, SPI_FILL_CHAR);
    LED_ON();
}

static inline void sd_spi_deselect(sd_card_t *sd_card_p) {
    gpio_put(sd_card_p->spi_if.ss_gpio, 1);
    LED_OFF();
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    sd_spi_write(sd_card_p, SPI_FILL_CHAR);
}

static inline void sd_spi_lock(sd_card_t *sd_card_p) {
    spi_lock(sd_card_p->spi_if.spi);
}
static inline void sd_spi_unlock(sd_card_t *sd_card_p) {
   spi_unlock(sd_card_p->spi_if.spi);
}

static inline void sd_spi_acquire(sd_card_t *sd_card_p) {
    sd_spi_lock(sd_card_p);
    sd_spi_select(sd_card_p);
}
static inline void sd_spi_release(sd_card_t *sd_card_p) {
    sd_spi_deselect(sd_card_p);
    sd_spi_unlock(sd_card_p);
}

#endif

/* [] END OF FILE */
