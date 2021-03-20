#pragma once

#include "sd_card.h"    
    
sd_card_t *sd_get_by_name(const char *const name);
sd_card_t *sd_get_by_num(size_t num);


/* [] END OF FILE */
