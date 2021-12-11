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

#ifndef _SD_SPI_H_
#define _SD_SPI_H_

#include <stdint.h>
#include "sd_card.h"

/* Transfer tx to SPI while receiving SPI to rx. 
tx or rx can be NULL if not important. */
bool sd_spi_transfer(sd_card_t *this, const uint8_t *tx, uint8_t *rx, size_t length);

uint8_t sd_spi_write(sd_card_t *this, const uint8_t value);
void sd_spi_acquire(sd_card_t *this);
void sd_spi_release(sd_card_t *this);
void sd_spi_go_low_frequency(sd_card_t *this);
void sd_spi_go_high_frequency(sd_card_t *this);
bool sd_spi_init(sd_card_t *this);

/* 
After power up, the host starts the clock and sends the initializing sequence on the CMD line. 
This sequence is a contiguous stream of logical ‘1’s. The sequence length is the maximum of 1msec, 
74 clocks or the supply-ramp-uptime; the additional 10 clocks 
(over the 64 clocks after what the card should be ready for communication) is
provided to eliminate power-up synchronization problems. 
*/
void sd_spi_send_initializing_sequence(sd_card_t * pSD);

#endif

/* [] END OF FILE */
