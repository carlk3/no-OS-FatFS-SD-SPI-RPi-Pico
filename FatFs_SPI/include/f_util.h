#pragma once
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

    const char *FRESULT_str(FRESULT i);
    FRESULT delete_node (
        TCHAR* path,    /* Path name buffer with the sub-directory to delete */
        UINT sz_buff,   /* Size of path name buffer (items) */
        FILINFO* fno    /* Name read buffer */
    );

#ifdef __cplusplus
}
#endif
