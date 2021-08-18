#pragma once

#include "ff.h"
#include "sd_card.h"    
    
#ifdef __cplusplus
extern "C" {
#endif

    size_t const sd_get_num();
    sd_card_t *const sd_get_by_num(size_t num);
    sd_card_t *const sd_get_by_name(const char *const name);
    size_t const spi_get_num();
    spi_t *const spi_get_by_num(size_t num);
    FATFS *const get_fs_by_name(const char *name);

#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
