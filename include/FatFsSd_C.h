/* FatFsSd_C.h
Copyright 2023 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
#pragma once

// #include "pico/stdlib.h"
// #include "hardware/sync.h"
// #include "pico/sync.h"
// #include "hardware/gpio.h"
//
#include "../src/ff15/source/ff.h"
//
#include "../src/ff15/source/diskio.h" /* Declarations of disk functions */
#include "../src/include/f_util.h"
#include "../src/include/rtc.h"
#include "../src/sd_driver/sd_card.h"
#include "../src/sd_driver/SDIO/rp2040_sdio.h"
#include "../src/sd_driver/SPI/spi.h"
#include "../src/sd_driver/hw_config.h"