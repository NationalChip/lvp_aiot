#include <stdio.h>
#include <string.h>
#include <board_config.h>

#include <driver/gx_irq.h>
#include <driver/gx_snpu.h>
#include <driver/gx_flash.h>

#include "../lvp_custom_space.h"
#include "lvp_close_recognize.h"

#define LOG_TAG "[CLOSE_RECOGNIZE]"
#define CAL_ARRAY_SIZE(x) (sizeof(x) / sizeof(int))

static CLOSE_KWS_INFO close_kws_info;

#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
static int WritCloseDataInfo(void)
{
    uint32_t irq_state = gx_lock_irq_save();
    gx_snpu_pause();

    GX_FLASH_DEV *flash = gx_spi_flash_probe(CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    unsigned int flash_size = gx_spi_flash_getinfo(flash, GX_FLASH_CHIP_SIZE);
    unsigned char otp[5] = {0};

    if (gx_spi_flash_otp_read(flash, 0, otp, 5) < 0) {
        printf(LOG_TAG "read flash otp error\n");
        gx_snpu_resume();
        gx_unlock_irq_restore(irq_state);
        return -1;
    }

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) {
        gx_spi_flash_write_protect_unlock(flash);
    }

    if (gx_spinor_flash_erasedata(CLOSE_RECOGNIZE_ADDR, FLASH_BLOCK_SIZE) < 0) {
        printf(LOG_TAG "erase flash error\n");
        gx_snpu_resume();
        gx_unlock_irq_restore(irq_state);
        return -1;
    }

    if (gx_spinor_flash_pageprogram(CLOSE_RECOGNIZE_ADDR,
                                    (unsigned char*)&close_kws_info.need_write_info,
                                    (1 + close_kws_info.need_write_info.need_write_num) * sizeof(int)) < 0) {
        printf(LOG_TAG "write close info error!");
        gx_snpu_resume();
        gx_unlock_irq_restore(irq_state);
        return -1;
    }

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) {
        unsigned int actual_protect = 0;
        gx_spi_flash_write_protect_status(flash, &actual_protect);
        if (actual_protect != (flash_size + 4 * 1024)) {
            unsigned long protect = flash_size + 4 * 1024;
            gx_spi_flash_write_protect_lock(flash, protect);
        }
    }

    gx_snpu_resume();
    gx_unlock_irq_restore(irq_state);

    return 0;
}
#endif

#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
static void ClearNoWriteInfo(void)
{
    close_kws_info.no_write_info.no_write_num = 0;
    memset(close_kws_info.no_write_info.no_write_kv, 0, sizeof(close_kws_info.no_write_info.no_write_kv));
}
#endif

#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
static void ClearNeedWriteInfo(void)
{
    close_kws_info.need_write_info.need_write_num = 0;
    memset(close_kws_info.need_write_info.need_write_kv, 0, sizeof(close_kws_info.need_write_info.need_write_kv));
    WritCloseDataInfo();
}
#endif

static int CheckArrayOverflow(int array_len, int write_flash_flag)
{
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    if (write_flash_flag == 0 && array_len > CAL_ARRAY_SIZE(close_kws_info.no_write_info.no_write_kv)) {
        printf(LOG_TAG "no_write_kv space overflow\n");
        return -1;
    }
#endif
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    if (write_flash_flag == 1 && array_len > CAL_ARRAY_SIZE(close_kws_info.need_write_info.need_write_kv)) {
        printf(LOG_TAG "need_write_kv space overflow\n");
        return -1;
    }
#endif
    return 0;
}

int LvpClearAllCloseInfo(void)
{
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    ClearNeedWriteInfo();
#endif
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    ClearNoWriteInfo();
#endif

    return 0;
}

int LvpBatchProcesCmdRecognize(int *kws_value_array, int array_len, int write_flash_flag)
{
    if (kws_value_array == NULL) {
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
        ClearNeedWriteInfo();
#endif
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
        ClearNoWriteInfo();
#endif
        return 0;
    }

    if (CheckArrayOverflow(array_len, write_flash_flag) != 0) {
        return -1;
    }

    if (write_flash_flag == 0) {
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
        memset(close_kws_info.no_write_info.no_write_kv, 0, sizeof(close_kws_info.no_write_info.no_write_kv));
        close_kws_info.no_write_info.no_write_num = array_len;
        memcpy(close_kws_info.no_write_info.no_write_kv, kws_value_array, array_len * sizeof(int));
#else
        printf(LOG_TAG "When write_flash_flag is 0, NO_NEED_WRITE_FLASH_CLOSE_KV_NUM cannot be set to 0\n");
        return -1;
#endif
    } else {
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
        memset(close_kws_info.no_write_info.no_write_kv, 0, sizeof(close_kws_info.no_write_info.no_write_kv));
        close_kws_info.need_write_info.need_write_num = array_len;
        memcpy(close_kws_info.need_write_info.need_write_kv, kws_value_array, array_len * sizeof(int));
        WritCloseDataInfo();
#else
        printf(LOG_TAG "When write_flash_flag is 1, NEED_WRITE_FLASH_CLOSE_KV_NUM cannot be set to 0\n");
        return -1;
#endif
    }

    return 0;
}

int LvpCloseCmdRecognize(int kws_value, int write_flash_flag)
{
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    if (write_flash_flag == 0 && close_kws_info.no_write_info.no_write_num + 1 > CAL_ARRAY_SIZE(close_kws_info.no_write_info.no_write_kv)) {
        printf(LOG_TAG "no_write_kv space overflow\n");
        return -1;
    }
#endif

#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    for (int i = 0; i < close_kws_info.no_write_info.no_write_num; i++) {
        if (close_kws_info.no_write_info.no_write_kv[i] == kws_value) {
            if (write_flash_flag == 1) {
# if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
                LvpOpenCmdRecognize(kws_value);
                break;
# endif
            } else {
                return 1;
            }
        }
    }
#endif

#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    if (write_flash_flag == 1 && close_kws_info.need_write_info.need_write_num + 1 > CAL_ARRAY_SIZE(close_kws_info.need_write_info.need_write_kv)) {
        printf(LOG_TAG "need_write_kv space overflow\n");
        return -1;
    }
    for (int i = 0; i < close_kws_info.need_write_info.need_write_num; i++) {
        if (close_kws_info.need_write_info.need_write_kv[i] == kws_value) {
            if (write_flash_flag == 0) {
                LvpOpenCmdRecognize(kws_value);
                break;
            } else {
                return 1;
            }
        }
    }
#endif

    if (write_flash_flag == 0) {
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
        close_kws_info.no_write_info.no_write_kv[close_kws_info.no_write_info.no_write_num++] = kws_value;
#else
        printf(LOG_TAG "When write_flash_flag is 0, NO_NEED_WRITE_FLASH_CLOSE_KV_NUM cannot be set to 0\n");
        return -1;
#endif
    } else {
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
        close_kws_info.need_write_info.need_write_kv[close_kws_info.need_write_info.need_write_num++] = kws_value;
        return WritCloseDataInfo();
#else
        printf(LOG_TAG "When write_flash_flag is 1, NEED_WRITE_FLASH_CLOSE_KV_NUM cannot be set to 0\n");
        return -1;
#endif
    }

    return 0;
}

int LvpOpenCmdRecognize(int kws_value)
{
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    for (int i = 0; i < close_kws_info.no_write_info.no_write_num; i++) {
        if (close_kws_info.no_write_info.no_write_kv[i] == kws_value) {
            memmove(&close_kws_info.no_write_info.no_write_kv[i], &close_kws_info.no_write_info.no_write_kv[i + 1],
                    (close_kws_info.no_write_info.no_write_num - i - 1) * sizeof(int));
            close_kws_info.no_write_info.no_write_num--;
            return 0;
        }
    }
#endif
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    for (int i = 0; i < close_kws_info.need_write_info.need_write_num; i++) {
        if (close_kws_info.need_write_info.need_write_kv[i] == kws_value) {
            memmove(&close_kws_info.need_write_info.need_write_kv[i], &close_kws_info.need_write_info.need_write_kv[i + 1],
                    (close_kws_info.need_write_info.need_write_num - i - 1) * sizeof(int));
            close_kws_info.need_write_info.need_write_num--;
            return WritCloseDataInfo();
        }
    }
#endif

    printf(LOG_TAG "kws_value[%d] is not closed\n", kws_value);
    return -1;
}

int LvpGetCloseStatus(int kws_value)
{
#if CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM
    for (int i = 0; i < close_kws_info.no_write_info.no_write_num; i++) {
        if (close_kws_info.no_write_info.no_write_kv[i] == kws_value) {
            return 1;
        }
    }
#endif

#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    for (int i = 0; i < close_kws_info.need_write_info.need_write_num; i++) {
        if (close_kws_info.need_write_info.need_write_kv[i] == kws_value) {
            return 1;
        }
    }
#endif

    return 0;
}

int LvpInitCloseCmdRecognize(void)
{
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    unsigned char *s_custom_space_addr = CUSTOM_TO_XIP_ADDR(CLOSE_RECOGNIZE_ADDR);
    close_kws_info.need_write_info.need_write_num = *(int*)s_custom_space_addr;

    if (close_kws_info.need_write_info.need_write_num < 0) {
        printf(LOG_TAG "The custom spatial location data is not clean!!![%d]\n", close_kws_info.need_write_info.need_write_num);
    }

    printf(LOG_TAG "Close the recognition instruction word kv list: [num: %d]\n", close_kws_info.need_write_info.need_write_num);

    for (int i = 0; i < close_kws_info.need_write_info.need_write_num; i++) {
        close_kws_info.need_write_info.need_write_kv[i] = *((int*)(s_custom_space_addr + ((i + 1) * sizeof(int))));
        printf(LOG_TAG "unnecessary_kv[%d]: %d\n", i, close_kws_info.need_write_info.need_write_kv[i]);
    }
#endif

    return 0;
}
