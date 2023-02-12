// Driver for accessing SD card in SDIO mode on RP2040.

#include "ZuluSCSI_platform.h"

#ifdef SD_USE_SDIO

#include <assert.h>
#include <stdint.h>
#include <string.h>
//
#include <hardware/gpio.h>
//
#include "diskio.h"
#include "my_debug.h"
#include "rp2040_sdio.h"
#include "rp2040_sdio.pio.h"
#include "SdioCard.h"
#include "util.h"

// #define azlog(...)
// #define azdbg(...)

//FIXME
#define azdbg(arg1, ...) {\
    printf("%s,%d: %s\n", __func__, __LINE__, arg1); \
}
#define azlog azdbg

static uint32_t g_sdio_ocr; // Operating condition register from card
static uint32_t g_sdio_rca; // Relative card address
static cid_t g_sdio_cid;
static csd_t g_sdio_csd;
static int g_sdio_error_line;
static sdio_status_t g_sdio_error;
static uint32_t g_sdio_dma_buf[128];
static uint32_t g_sdio_sector_count;

#define checkReturnOk(call) ((g_sdio_error = (call)) == SDIO_OK ? true : logSDError(__LINE__))

static bool logSDError(int line)
{
    g_sdio_error_line = line;
    azlog("SDIO SD card error on line ", line, ", error code ", (int)g_sdio_error);
    return false;
}

// Callback used by SCSI code for simultaneous processing
static sd_callback_t m_stream_callback;
static const uint8_t *m_stream_buffer;
static uint32_t m_stream_count;
static uint32_t m_stream_count_start;

void azplatform_set_sd_callback(sd_callback_t func, const uint8_t *buffer)
{
    m_stream_callback = func;
    m_stream_buffer = buffer;
    m_stream_count = 0;
    m_stream_count_start = 0;
}

static sd_callback_t get_stream_callback(const uint8_t *buf, uint32_t count, const char *accesstype, uint32_t sector)
{
    m_stream_count_start = m_stream_count;

    if (m_stream_callback)
    {
        if (buf == m_stream_buffer + m_stream_count)
        {
            m_stream_count += count;
            return m_stream_callback;
        }
        else
        {
            azdbg("SD card ", accesstype, "(", (int)sector,
                  ") slow transfer, buffer", (uint32_t)buf, " vs. ", (uint32_t)(m_stream_buffer + m_stream_count));
            return NULL;
        }
    }
    
    return NULL;
}

bool sd_sdio_begin(sd_card_t *sd_card_p)
{
    uint32_t reply;
    sdio_status_t status;
    
    // Initialize at 1 MHz clock speed
    rp2040_sdio_init(sd_card_p, 25);

    // Establish initial connection with the card
    for (int retries = 0; retries < 5; retries++)
    {
        delayMicroseconds(1000);
        reply = 0;
        rp2040_sdio_command_R1(sd_card_p, CMD0, 0, NULL); // GO_IDLE_STATE
        status = rp2040_sdio_command_R1(sd_card_p, CMD8, 0x1AA, &reply); // SEND_IF_COND

        if (status == SDIO_OK && reply == 0x1AA)
        {
            break;
        }
    }

    if (reply != 0x1AA || status != SDIO_OK)
    {
        // azdbg("SDIO not responding to CMD8 SEND_IF_COND, status ", (int)status, " reply ", reply);
        printf("%s,%d SDIO not responding to CMD8 SEND_IF_COND, status 0x%x reply 0x%lx\n", 
            __func__, __LINE__, status, reply);
        return false;
    }

    // Send ACMD41 to begin card initialization and wait for it to complete
    // uint32_t start = millis();
    absolute_time_t timeout_time = make_timeout_time_ms(1000);
    do {
        if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD55, 0, &reply)) || // APP_CMD
            !checkReturnOk(rp2040_sdio_command_R3(sd_card_p, ACMD41, 0xD0040000, &g_sdio_ocr))) // 3.0V voltage
            // !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, ACMD41, 0xC0100000, &g_sdio_ocr)))
        {
            return false;
        }

        // if ((uint32_t)(millis() - start) > 1000)
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0)
        {
            azlog("SDIO card initialization timeout");
            return false;
        }
    } while (!(g_sdio_ocr & (1 << 31)));

    // Get CID
    if (!checkReturnOk(rp2040_sdio_command_R2(sd_card_p, CMD2, 0, (uint8_t*)&g_sdio_cid)))
    {
        azdbg("SDIO failed to read CID");
        return false;
    }

    // Get relative card address
    if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD3, 0, &g_sdio_rca)))
    {
        azdbg("SDIO failed to get RCA");
        return false;
    }

    // Get CSD
    if (!checkReturnOk(rp2040_sdio_command_R2(sd_card_p, CMD9, g_sdio_rca, (uint8_t*)&g_sdio_csd)))
    {
        azdbg("SDIO failed to read CSD");
        return false;
    }

    g_sdio_sector_count = sd_sdio_sectorCount(sd_card_p);

    // Select card
    if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD7, g_sdio_rca, &reply)))
    {
        azdbg("SDIO failed to select card");
        return false;
    }

    // Set 4-bit bus mode
    if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD55, g_sdio_rca, &reply)) ||
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, ACMD6, 2, &reply)))
    {
        azdbg("SDIO failed to set bus width");
        return false;
    }
    if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, 16, 512, &reply))) // SET_BLOCKLEN
    {
        printf("%s,%d SDIO failed to set BLOCKLEN\n", __func__, __LINE__);
        return false;
    }

    // Increase to 25 MHz clock rate
    // Actually, clk_sys / CLKDIV (from rp2040_sdio.pio),
    // So, say, 125 MHz / 4 = 31.25 MHz
    rp2040_sdio_init(sd_card_p, 1);

    return true;
}

uint8_t sd_sdio_errorCode() // const
{
    return g_sdio_error;
}

uint32_t sd_sdio_errorData() // const
{
    return 0;
}

uint32_t sd_sdio_errorLine() // const
{
    return g_sdio_error_line;
}

bool sd_sdio_isBusy(sd_card_t *sd_card_p) 
{
    // return (sio_hw->gpio_in & (1 << SDIO_D0)) == 0;
    return (sio_hw->gpio_in & (1 << sd_card_p->sdio_if.D0_gpio)) == 0;
}

uint32_t sd_sdio_kHzSdClk(sd_card_t *sd_card_p)
{
    return 0;
}

bool sd_sdio_readCID(sd_card_t *sd_card_p, cid_t* cid)
{
    *cid = g_sdio_cid;
    return true;
}

bool sd_sdio_readCSD(sd_card_t *sd_card_p, csd_t* csd)
{
    *csd = g_sdio_csd;
    return true;
}

bool sd_sdio_readOCR(sd_card_t *sd_card_p, uint32_t* ocr)
{
    // SDIO mode does not have CMD58, but main program uses this to
    // poll for card presence. Return status register instead.
    return checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD13, g_sdio_rca, ocr));
}

bool sd_sdio_readData(sd_card_t *sd_card_p, uint8_t* dst)
{
    azlog("sd_sdio_readData() called but not implemented!");
    return false;
}

// bool sd_sdio_readStart(sd_card_t *sd_card_p, uint32_t sector)
// {
//     azlog("sd_sdio_readStart() called but not implemented!");
//     return false;
// }

// bool sd_sdio_readStop(sd_card_t *sd_card_p)
// {
//     azlog("sd_sdio_readStop() called but not implemented!");
//     return false;
// }

uint64_t sd_sdio_sectorCount(sd_card_t *sd_card_p)
{
    // return g_sdio_csd.capacity();
    return CSD_capacity(&g_sdio_csd);
}

uint32_t sd_sdio_status(sd_card_t *sd_card_p)
{
    uint32_t reply;
    if (checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD13, g_sdio_rca, &reply)))
        return reply;
    else
        return 0;
}

bool sd_sdio_stopTransmission(sd_card_t *sd_card_p, bool blocking)
{
    uint32_t reply;
    if (!checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD12, 0, &reply)))
    {
        return false;
    }

    if (!blocking)
    {
        return true;
    }
    else
    {
        // uint32_t end = millis() + 100;
        absolute_time_t timeout_time = make_timeout_time_ms(200); // CK3: doubled
        // while (millis() < end && sd_sdio_isBusy(sd_card_p))
        while (0 < absolute_time_diff_us(get_absolute_time(), timeout_time) && sd_sdio_isBusy(sd_card_p))
        {
            if (m_stream_callback)
            {
                m_stream_callback(m_stream_count);
            }
        }
        if (sd_sdio_isBusy(sd_card_p))
        {
            azlog("sd_sdio_stopTransmission() timeout");
            return false;
        }
        else
        {
            return true;
        }
    }
}

bool sd_sdio_syncDevice(sd_card_t *sd_card_p)
{
    return true;
}

uint8_t sd_sdio_type(sd_card_t *sd_card_p) // const
{
    if (g_sdio_ocr & (1 << 30))
        return SD_CARD_TYPE_SDHC;
    else
        return SD_CARD_TYPE_SD2;
}

bool sd_sdio_writeData(sd_card_t *sd_card_p, const uint8_t* src)
{
    azlog("sd_sdio_writeData() called but not implemented!");
    return false;
}

// bool sd_sdio_writeStart(sd_card_t *sd_card_p, uint32_t sector)
// {
//     azlog("sd_sdio_writeStart() called but not implemented!");
//     return false;
// }

// bool sd_sdio_writeStop(sd_card_t *sd_card_p)
// {
//     azlog("sd_sdio_writeStop() called but not implemented!");
//     return false;
// }

bool sd_sdio_erase(sd_card_t *sd_card_p, uint32_t firstSector, uint32_t lastSector)
{
    azlog("sd_sdio_erase() not implemented");
    return false;
}

bool sd_sdio_cardCMD6(sd_card_t *sd_card_p, uint32_t arg, uint8_t* status) {
    azlog("sd_sdio_cardCMD6() not implemented");
    return false;
}

bool sd_sdio_readSCR(sd_card_t *sd_card_p, scr_t* scr) {
    azlog("sd_sdio_readSCR() not implemented");
    return false;
}

/* Writing and reading, with progress callback */

bool sd_sdio_writeSector(sd_card_t *sd_card_p, uint32_t sector, const uint8_t* src)
{
    if (((uint32_t)src & 3) != 0)
    {
        // Buffer is not aligned, need to memcpy() the data to a temporary buffer.
        memcpy(g_sdio_dma_buf, src, sizeof(g_sdio_dma_buf));
        src = (uint8_t*)g_sdio_dma_buf;
    }

    // If possible, report transfer status to application through callback.
    sd_callback_t callback = get_stream_callback(src, 512, "writeSector", sector);

    uint32_t reply;
    if (/* !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, 16, 512, &reply)) || // SET_BLOCKLEN */
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD24, sector, &reply)) || // WRITE_BLOCK
        !checkReturnOk(rp2040_sdio_tx_start(sd_card_p, src, 1))) // Start transmission
    {
        return false;
    }

    do {
        uint32_t bytes_done;
        g_sdio_error = rp2040_sdio_tx_poll(sd_card_p, &bytes_done);

        if (callback)
        {
            callback(m_stream_count_start + bytes_done);
        }
    } while (g_sdio_error == SDIO_BUSY);

    if (g_sdio_error != SDIO_OK)
    {
        azlog("sd_sdio_writeSector(", sector, ") failed: ", (int)g_sdio_error);
    }

    return g_sdio_error == SDIO_OK;
}

bool sd_sdio_writeSectors(sd_card_t *sd_card_p, uint32_t sector, const uint8_t* src, size_t n)
{
    if (((uint32_t)src & 3) != 0)
    {
        // Unaligned write, execute sector-by-sector
        for (size_t i = 0; i < n; i++)
        {
            if (!sd_sdio_writeSector(sd_card_p, sector + i, src + 512 * i))
            {
                return false;
            }
        }
        return true;
    }

    sd_callback_t callback = get_stream_callback(src, n * 512, "writeSectors", sector);

    uint32_t reply;
    if (/* !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, 16, 512, &reply)) || // SET_BLOCKLEN */
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD55, g_sdio_rca, &reply)) || // APP_CMD
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, ACMD23, n, &reply)) || // SET_WR_CLK_ERASE_COUNT
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD25, sector, &reply)) || // WRITE_MULTIPLE_BLOCK
        !checkReturnOk(rp2040_sdio_tx_start(sd_card_p, src, n))) // Start transmission
    {
        return false;
    }

    do {
        uint32_t bytes_done;
        g_sdio_error = rp2040_sdio_tx_poll(sd_card_p, &bytes_done);

        if (callback)
        {
            callback(m_stream_count_start + bytes_done);
        }
    } while (g_sdio_error == SDIO_BUSY);

    if (g_sdio_error != SDIO_OK)
    {
        azlog("sd_sdio_writeSectors(", sector, ",...,", (int)n, ") failed: ", (int)g_sdio_error);
        sd_sdio_stopTransmission(sd_card_p, true);
        return false;
    }
    else
    {
        return sd_sdio_stopTransmission(sd_card_p, true);
    }
}

bool sd_sdio_readSector(sd_card_t *sd_card_p, uint32_t sector, uint8_t* dst)
{
    uint8_t *real_dst = dst;
    if (((uint32_t)dst & 3) != 0)
    {
        // Buffer is not aligned, need to memcpy() the data from a temporary buffer.
        dst = (uint8_t*)g_sdio_dma_buf;
    }

    sd_callback_t callback = get_stream_callback(dst, 512, "readSector", sector);

    uint32_t reply;
    if (/* !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, 16, 512, &reply)) || // SET_BLOCKLEN */
        !checkReturnOk(rp2040_sdio_rx_start(sd_card_p, dst, 1)) || // Prepare for reception
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD17, sector, &reply))) // READ_SINGLE_BLOCK
    {
        return false;
    }

    do {
        uint32_t bytes_done;
        g_sdio_error = rp2040_sdio_rx_poll(sd_card_p, &bytes_done);

        if (callback)
        {
            callback(m_stream_count_start + bytes_done);
        }
    } while (g_sdio_error == SDIO_BUSY);

    if (g_sdio_error != SDIO_OK)
    {
        // azlog("sd_sdio_readSector(", sector, ") failed: ", (int)g_sdio_error);
        printf("%s,%d sd_sdio_readSector(%lu) failed: %d\n", 
            __func__, __LINE__, sector, g_sdio_error);
    }

    if (dst != real_dst)
    {
        memcpy(real_dst, g_sdio_dma_buf, sizeof(g_sdio_dma_buf));
    }

    return g_sdio_error == SDIO_OK;
}

bool sd_sdio_readSectors(sd_card_t *sd_card_p, uint32_t sector, uint8_t* dst, size_t n)
{
    if (((uint32_t)dst & 3) != 0 || sector + n >= g_sdio_sector_count)
    {
        // Unaligned read or end-of-drive read, execute sector-by-sector
        for (size_t i = 0; i < n; i++)
        {
            if (!sd_sdio_readSector(sd_card_p, sector + i, dst + 512 * i))
            {
                return false;
            }
        }
        return true;
    }

    sd_callback_t callback = get_stream_callback(dst, n * 512, "readSectors", sector);

    uint32_t reply;
    if (/* !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, 16, 512, &reply)) || // SET_BLOCKLEN */
        !checkReturnOk(rp2040_sdio_rx_start(sd_card_p, dst, n)) || // Prepare for reception
        !checkReturnOk(rp2040_sdio_command_R1(sd_card_p, CMD18, sector, &reply))) // READ_MULTIPLE_BLOCK
    {
        return false;
    }

    do {
        uint32_t bytes_done;
        g_sdio_error = rp2040_sdio_rx_poll(sd_card_p, &bytes_done);

        if (callback)
        {
            callback(m_stream_count_start + bytes_done);
        }
    } while (g_sdio_error == SDIO_BUSY);

    if (g_sdio_error != SDIO_OK)
    {
        // azlog("sd_sdio_readSectors(", sector, ",...,", (int)n, ") failed: ", (int)g_sdio_error);
        printf("sd_sdio_readSectors(%ld,...,%d)  failed: %d\n", sector, n, g_sdio_error);
        sd_sdio_stopTransmission(sd_card_p, true);
        return false;
    }
    else
    {
        return sd_sdio_stopTransmission(sd_card_p, true);
    }
}

static int sd_sdio_init(sd_card_t *sd_card_p) {
    // bool sd_sdio_begin(sd_card_t *sd_card_p);
    bool rc = sd_sdio_begin(sd_card_p);
    if (rc) {
        // The card is now initialized
        sd_card_p->m_Status &= ~STA_NOINIT;
    }
    return sd_card_p->m_Status;
}
static int sd_sdio_write_blocks(sd_card_t *sd_card_p, const uint8_t *buffer,
                                uint64_t ulSectorNumber, uint32_t blockCnt) {
    // bool sd_sdio_writeSectors(sd_card_t *sd_card_p, uint32_t sector, const uint8_t* src, size_t ns);
    bool rc;
    if (1 == blockCnt)
        rc = sd_sdio_writeSector(sd_card_p, ulSectorNumber, buffer);
    else
        rc = sd_sdio_writeSectors(sd_card_p, ulSectorNumber, buffer, blockCnt);
    if (rc)
        return SD_BLOCK_DEVICE_ERROR_NONE;
    else
        return SD_BLOCK_DEVICE_ERROR_WRITE;                                    
}
static int sd_sdio_read_blocks(sd_card_t *sd_card_p, uint8_t *buffer, uint64_t ulSectorNumber,
                               uint32_t ulSectorCount) {
    // bool sd_sdio_readSectors(sd_card_t *sd_card_p, uint32_t sector, uint8_t* dst, size_t n)
    bool rc;
    if (1 == ulSectorCount) 
        rc = sd_sdio_readSector(sd_card_p, ulSectorNumber, buffer);
    else        
        rc= sd_sdio_readSectors(sd_card_p, ulSectorNumber, buffer, ulSectorCount);
    if (rc)
        return SD_BLOCK_DEVICE_ERROR_NONE;
    else
        return SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
}

// Helper function to configure whole GPIO in one line
static void gpio_conf(uint gpio, enum gpio_function fn, bool pullup, bool pulldown, bool output, bool initial_state, bool fast_slew)
{
    gpio_put(gpio, initial_state);
    gpio_set_dir(gpio, output);
    gpio_set_pulls(gpio, pullup, pulldown);
    gpio_set_function(gpio, fn);

    if (fast_slew)
    {
        padsbank0_hw->io[gpio] |= PADS_BANK0_GPIO0_SLEWFAST_BITS;
    }
}

void sd_sdio_ctor(sd_card_t *sd_card_p) {
    assert(!sd_card_p->sdio_if.CLK_gpio);
    assert(!sd_card_p->sdio_if.D1_gpio);
    assert(!sd_card_p->sdio_if.D2_gpio);
    assert(!sd_card_p->sdio_if.D3_gpio);

    sd_card_p->sdio_if.CLK_gpio = (sd_card_p->sdio_if.D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
    sd_card_p->sdio_if.D1_gpio = sd_card_p->sdio_if.D0_gpio + 1;
    sd_card_p->sdio_if.D2_gpio = sd_card_p->sdio_if.D0_gpio + 2;
    sd_card_p->sdio_if.D3_gpio = sd_card_p->sdio_if.D0_gpio + 3;

    sd_card_p->m_Status = STA_NOINIT;

    sd_card_p->init = sd_sdio_init;
    sd_card_p->write_blocks = sd_sdio_write_blocks;
    sd_card_p->read_blocks = sd_sdio_read_blocks;
    sd_card_p->get_num_sectors = sd_sdio_sectorCount;
    sd_card_p->sd_readCID = sd_sdio_readCID;

    //        pin                          function        pup   pdown  out    state fast
    gpio_conf(sd_card_p->sdio_if.CLK_gpio, GPIO_FUNC_PIO1, true, false, true,  true, true);
    gpio_conf(sd_card_p->sdio_if.CMD_gpio, GPIO_FUNC_PIO1, true, false, true,  true, true);
    gpio_conf(sd_card_p->sdio_if.D0_gpio,  GPIO_FUNC_PIO1, true, false, false, true, true);
    gpio_conf(sd_card_p->sdio_if.D1_gpio,  GPIO_FUNC_PIO1, true, false, false, true, true);
    gpio_conf(sd_card_p->sdio_if.D2_gpio,  GPIO_FUNC_PIO1, true, false, false, true, true);
    gpio_conf(sd_card_p->sdio_if.D3_gpio,  GPIO_FUNC_PIO1, true, false, false, true, true);

    if (sd_card_p->use_card_detect) {
        gpio_init(sd_card_p->card_detect_gpio);
        gpio_pull_up(sd_card_p->card_detect_gpio);
        gpio_set_dir(sd_card_p->card_detect_gpio, GPIO_IN);
    }
}

#endif