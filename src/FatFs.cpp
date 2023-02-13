/* FatFsSd.cpp
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

#include "FatFs.h"

/*
    See FatFs - Generic FAT Filesystem Module, "Application Interface",
    http://elm-chan.org/fsw/ff/00index_e.html
 */

std::vector<FatFs_Spi> FatFs::Spis;
std::vector<FatFs_SdCard> FatFs::SdCards;

FRESULT FatFs_SdCard::mount() {
    return f_mount(&m_sd_card.fatfs, m_sd_card.pcName, 1);
}
FRESULT FatFs_SdCard::unmount() {
    return f_unmount(m_sd_card.pcName);
}

FRESULT FatFs::chdrive(const TCHAR *path) {
    return f_chdrive(path);
}

FRESULT FatFs_File::open(const TCHAR *path, BYTE mode) { /* Open or create a file */
    return f_open(&fil, path, mode);
}
FRESULT FatFs_File::close() { /* Close an open file object */
    return f_close(&fil);
}
FRESULT FatFs_File::read(void *buff, UINT btr, UINT *br) { /* Read data from the file */
    return f_read(&fil, buff, btr, br);
}
FRESULT FatFs_File::write(const void *buff, UINT btw, UINT *bw) { /* Write data to the file */
    return f_write(&fil, buff, btw, bw);
}
FRESULT FatFs_File::lseek(FSIZE_t ofs) { /* Move file pointer of the file object */
    return f_lseek(&fil, ofs);
}
FRESULT FatFs_File::truncate() { /* Truncate the file */
    return f_truncate(&fil);
}
FRESULT FatFs_File::sync() { /* Flush cached data of the writing file */
    return f_sync(&fil);
}
int FatFs_File::putc(TCHAR c) { /* Put a character to the file */
    return f_putc(c, &fil);
}
int FatFs_File::puts(const TCHAR *str) { /* Put a string to the file */
    return f_puts(str, &fil);
}
// int FatFs_File::printf(const TCHAR *str, ...){                 /* Put a formatted string to the file */
//     return f_printf(&fil, str, ...);
// }
TCHAR *FatFs_File::gets(TCHAR *buff, int len) { /* Get a string from the file */
    return f_gets(buff, len, &fil);
}
bool FatFs_File::eof() {
    return f_eof(&fil);
}
BYTE FatFs_File::error() {
    return f_error(&fil);
}
FSIZE_t FatFs_File::tell() {
    return f_tell(&fil);
}
FSIZE_t FatFs_File::size() {
    return f_size(&fil);
}
int FatFs_File::rewind() {
    return f_rewind(&fil);
}

size_t spi_get_num() { return FatFs::Spi_get_num(); }
spi_t *spi_get_by_num(size_t num) { return &FatFs::Spi_get_by_num(num)->m_spi; }
size_t sd_get_num() { return FatFs::SdCard_get_num(); }
sd_card_t *sd_get_by_num(size_t num) { return &FatFs::SdCard_get_by_num(num)->m_sd_card; }
