#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    int lliot(size_t pnum);
    void ls(const char *dir);
    void simple();
    void bench(char const* logdrv);
    void big_file_test(const char *const pathname, size_t size,
                            uint32_t seed);
    void vCreateAndVerifyExampleFiles(const char *pcMountPath);
    void vStdioWithCWDTest(const char *pcMountPath);
    bool process_logger();
#ifdef __cplusplus
}
#endif
