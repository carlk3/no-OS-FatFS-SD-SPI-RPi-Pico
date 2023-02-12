/* sd_spi.c
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
#include <stdint.h>
#include <string.h>
//
#include "hardware/gpio.h"
//
#include "my_debug.h"
#include "spi.h"
//
#include "sd_spi.h"

#define TRACE_PRINTF(fmt, args...)
// #define TRACE_PRINTF printf

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void sd_spi_go_high_frequency(sd_card_t *sd_card_p) {
    uint actual = spi_set_baudrate(sd_card_p->spi_if.spi->hw_inst, sd_card_p->spi_if.spi->baud_rate);
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, (long)actual);
}
void sd_spi_go_low_frequency(sd_card_t *sd_card_p) {
    uint actual = spi_set_baudrate(sd_card_p->spi_if.spi->hw_inst, 400 * 1000); // Actual frequency: 398089
    TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, (long)actual);
}
#pragma GCC diagnostic pop

static void sd_spi_lock(sd_card_t *sd_card_p) {
    spi_lock(sd_card_p->spi_if.spi);
}
static void sd_spi_unlock(sd_card_t *sd_card_p) {
   spi_unlock(sd_card_p->spi_if.spi);
}

// Would do nothing if sd_card_p->spi_if.ss_gpio were set to GPIO_FUNC_SPI.
static void sd_spi_select(sd_card_t *sd_card_p) {
    gpio_put(sd_card_p->spi_if.ss_gpio, 0);
    // A fill byte seems to be necessary, sometimes:
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(sd_card_p->spi_if.spi->hw_inst, &fill, 1);
    LED_ON();
}

static void sd_spi_deselect(sd_card_t *sd_card_p) {
    gpio_put(sd_card_p->spi_if.ss_gpio, 1);
    LED_OFF();
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(sd_card_p->spi_if.spi->hw_inst, &fill, 1);
}
/* Some SD cards want to be deselected between every bus transaction */
void sd_spi_deselect_pulse(sd_card_t *sd_card_p) {
    sd_spi_deselect(sd_card_p);
    // tCSH Pulse duration, CS high 200 ns
    sd_spi_select(sd_card_p);
}
void sd_spi_acquire(sd_card_t *sd_card_p) {
    sd_spi_lock(sd_card_p);
    sd_spi_select(sd_card_p);
}

void sd_spi_release(sd_card_t *sd_card_p) {
    sd_spi_deselect(sd_card_p);
    sd_spi_unlock(sd_card_p);
}

/* Transfer tx to SPI while receiving SPI to rx. 
tx or rx can be NULL if not important. */
bool sd_spi_transfer(sd_card_t *sd_card_p, const uint8_t *tx, uint8_t *rx,
                     size_t length) {
    return spi_transfer(sd_card_p->spi_if.spi, tx, rx, length);
}

uint8_t sd_spi_write(sd_card_t *sd_card_p, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    uint8_t received = SPI_FILL_CHAR;
#if 0
    int num = spi_write_read_blocking(sd_card_p->spi_if.spi->hw_inst, &value, &received, 1);    
    myASSERT(1 == num);
#else
    bool success = spi_transfer(sd_card_p->spi_if.spi, &value, &received, 1);
    myASSERT(success);
#endif
    return received;
}

/* 
After power up, the host starts the clock and sends the initializing sequence on the CMD line. 
This sequence is a contiguous stream of logical ‘1’s. The sequence length is the maximum of 1msec, 
74 clocks or the supply-ramp-uptime; the additional 10 clocks 
(over the 64 clocks after what the card should be ready for communication) is
provided to eliminate power-up synchronization problems. 
*/
void sd_spi_send_initializing_sequence(sd_card_t * sd_card_p) {
    bool old_ss = gpio_get(sd_card_p->spi_if.ss_gpio);
    // Set DI and CS high and apply 74 or more clock pulses to SCLK:
    gpio_put(sd_card_p->spi_if.ss_gpio, 1);
    uint8_t ones[10];
    memset(ones, 0xFF, sizeof ones);
    absolute_time_t timeout_time = make_timeout_time_ms(1);
    do {
        sd_spi_transfer(sd_card_p, ones, NULL, sizeof ones);
    } while (0 < absolute_time_diff_us(get_absolute_time(), timeout_time));
    gpio_put(sd_card_p->spi_if.ss_gpio, old_ss);
}



/* [] END OF FILE */
