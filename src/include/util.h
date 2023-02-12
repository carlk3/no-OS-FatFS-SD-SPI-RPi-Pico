/* util.h
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
#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>    
#include <stdint.h>
#include "hardware/structs/scb.h"
#include "RP2040.h"
#include "my_debug.h"

// works with negative index
static inline int wrap_ix(int index, int n)
{
    return ((index % n) + n) % n;
}
static inline int mod_floor(int a, int n) {
    return ((a % n) + n) % n;
}

__attribute__((always_inline)) static inline uint32_t calculate_checksum(uint32_t const *p, size_t const size){
	uint32_t checksum = 0;
	for (uint32_t i = 0; i < (size/sizeof(uint32_t))-1; i++){
		checksum ^= *p;
		p++;
	}
	return checksum;
}

static inline void system_reset() {
    __NVIC_SystemReset();
}

static inline void dump_bytes(size_t num, uint8_t bytes[num]) {
    printf("     ");
    for (size_t j = 0; j < 16; ++j) {
        printf("%02hhx", j);
        if (j < 15)
            printf(" ");
        else {
            printf("\n");
        }
    }
    for (size_t i = 0; i < num; i += 16) {
        printf("%04x ", i);        
        for (size_t j = 0; j < 16 && i + j < num; ++j) {
            printf("%02hhx", bytes[i + j]);
            if (j < 15)
                printf(" ");
            else {
                printf("\n");
            }
        }
    }
    printf("\n");
    fflush(stdout);
}

#endif
/* [] END OF FILE */
