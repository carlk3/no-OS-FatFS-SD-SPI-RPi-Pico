/*

*/

#include <string.h>
//
#include "my_debug.h"
//
#include "hw_config.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */
#include "vector"

static std::vector<spi_t *> spis;
static std::vector<sd_card_t *> sd_cards;

size_t sd_get_num() { return sd_cards.size(); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return sd_cards[num];
    } else {
        return NULL;
    }
}
sd_card_t *sd_get_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_cards[i]->pcName, name)) return sd_cards[i];
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
size_t spi_get_num() { return spis.size(); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return spis[num];
    } else {
        return NULL;
    }
}
FATFS *get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_cards[i]->pcName, name)) return &sd_cards[i]->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
void add_spi(spi_t *spi) { spis.push_back(spi); }
void add_sd_card(sd_card_t *sd_card) { sd_cards.push_back(sd_card); }

/* [] END OF FILE */
