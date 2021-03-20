#include <stddef.h>
#include <string.h>
//
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"

typedef uint32_t DWORD;
typedef unsigned int UINT;

extern int test_diskio_initialize(const char *diskName);
extern int test_diskio(
    UINT ncyc,    /* Number of test cycles */
    DWORD *buff,  /* Pointer to the working buffer */
    UINT sz_buff, /* Size of the working buffer in unit of byte */
    DWORD lba_offset);

static const size_t n_tasks = 3;
static const size_t ncyc = SIZE_MAX;
static const size_t usStackSizeWords = 1024;
static const size_t priority = tskIDLE_PRIORITY + 1;

#define buff_sz 2048

typedef struct {
    size_t lba_offset;
    DWORD buff[buff_sz / sizeof(DWORD)];
    char task_name[configMAX_TASK_NAME_LEN];
    char disk_name[64];
} args_t;

static void LliotTask(void *arg) {
    args_t *p_args = arg;
    /* Check function/compatibility of the physical drive */
    int rc = test_diskio(ncyc, p_args->buff, buff_sz, p_args->lba_offset);
    configASSERT(!rc);
    vPortFree(arg);
    vTaskDelete(NULL);
}

void mt_lliot(const char *diskName) {
    for (size_t i = 0; i < n_tasks; ++i) {
        args_t *p_args = pvPortMalloc(sizeof(args_t));
        if (!p_args) {
            printf("%s: Couldn't allocate %zu bytes\n", __FUNCTION__,
                   sizeof(args_t));
            return;
        }
        p_args->lba_offset = i * 0x20;
        snprintf(p_args->task_name, sizeof p_args->task_name, "FS%zu", i);
        snprintf(p_args->disk_name, sizeof p_args->disk_name, "%s", diskName);
        int ec = test_diskio_initialize(p_args->disk_name);
        configASSERT(!ec);
        BaseType_t rc = xTaskCreate(LliotTask, p_args->task_name,
                                    usStackSizeWords, p_args, priority, NULL);
        configASSERT(pdPASS == rc);
    }
}

/*-----------------------------------------------------------*/
static BaseType_t mt_lliot_cmd(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString) {
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the first parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );

    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    mt_lliot(pcParameter);

    return pdFALSE;
}
const CLI_Command_Definition_t xMTLowLevIOTests = {
    "mtlliot", /* The command string to type. */
    "\nmtlliot <device name>:\n Multi-task Low Level I/O Driver Test\n"
    "\te.g.: \"mtlliot sd0\"\n",
    mt_lliot_cmd, /* The function to run. */
    1             /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
