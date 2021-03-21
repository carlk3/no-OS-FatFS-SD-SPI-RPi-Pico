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
/*

This file should be tailored to match the hardware design that is specified in
TopDesign.cysch and Design Wide Resources/Pins (the .cydwr file).

There should be one element of the spi[] array for each hardware SPI used.

There should be one element of the sd_cards[] array for each SD card slot.
The name is arbitrary. The rest of the constants will depend on the type of
socket, which SPI it is driven by, and how it is wired.

*/

#include <string.h>

// Make it easier to spot errors:
#include "hw_config.h"
#include "my_debug.h"

void spi0_dma_isr();

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spi[] = {  // One for each SPI.
    {
        .hw_inst = spi0,  // SPI component
        .miso_gpio = 19,
        .mosi_gpio = 16,
        .sck_gpio = 18,
        //.baud_rate = 1000 * 1000,  //DEBUG
        .baud_rate = 12500 * 1000,  // The limitation here is SPI slew rate.
        //.baud_rate = 25 * 1000 * 1000, // Actual frequency: 20833333. Has
        // worked for me.

        // Following attributes are dynamically assigned
        .dma_isr = spi0_dma_isr,
        .initialized = false,  // initialized flag
    }};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {.pcName = "0:",            // Name used to mount device
     .spi = &spi[0],             // Pointer to the SPI driving this card
     .ss_gpio = 17,              // The SPI slave select GPIO for this SD card
     .card_detect_gpio = 22,     // Card detect
     .card_detected_true = 1,    // What the GPIO read returns when a card is
                                 // present. Use -1 if there is no card detect.
     // Following attributes are dynamically assigned
     .m_Status = STA_NOINIT,
     .sectors = 0,
     .card_type = 0,
     }};

void spi0_dma_isr() { spi_irq_handler(&spi[0]); }

/* ********************************************************************** */
sd_card_t *sd_get_by_name(const char *const name) {
    size_t i;
    for (i = 0; i < sizeof(sd_cards) / sizeof(sd_cards[0]); ++i) {
        if (0 == strcmp(sd_cards[i].pcName, name)) break;
    }
    if (sizeof(sd_cards) / sizeof(sd_cards[0]) == i) {
        DBG_PRINTF("FF_SDDiskInit: unknown name %s\n", name);
        return NULL;
    }
    return &sd_cards[i];
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sizeof(sd_cards) / sizeof(sd_cards[0])) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
