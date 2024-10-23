#include <stdio.h>
#include <board_config.h>

#include <driver/gx_irq.h>
#include <driver/gx_snpu.h>
#include <driver/gx_flash.h>

#include "../lvp_custom_space.h"
#include "lvp_write_custom_data.h"

#define LOG_TAG "[WRITE_CUSTOM_DATA]"

int LvpWriteCustomData(unsigned char *data, int len)
{
    if (len > (WRITE_CUSTOM_DATA_LEN)) {
        printf("Too much custom data failed to save\n");
        return -1;
    }

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

    if (gx_spinor_flash_erasedata(WRITE_CUSTOM_DATA_ADDR, WRITE_CUSTOM_DATA_LEN) < 0) {
        printf(LOG_TAG "erase flash error\n");
        gx_snpu_resume();
        gx_unlock_irq_restore(irq_state);
        return -1;
    }

    if (gx_spinor_flash_pageprogram(WRITE_CUSTOM_DATA_ADDR, (unsigned char*)&len, sizeof(int)) < 0) {
        printf(LOG_TAG "write len info error!");
        gx_snpu_resume();
        gx_unlock_irq_restore(irq_state);
        return -1;
    }

    if (gx_spinor_flash_pageprogram(WRITE_CUSTOM_DATA_ADDR + sizeof(int), data, len) < 0) {
        printf(LOG_TAG "write data info error!");
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

unsigned char *LvpReadCustomData(int *data_len)
{
    unsigned char *custom_space_write_data_addr = CUSTOM_TO_XIP_ADDR(WRITE_CUSTOM_DATA_ADDR);
    int len = *(int*)(custom_space_write_data_addr);

    if (len == 0) {
        return NULL;
    }

    *data_len = len;
    return custom_space_write_data_addr + sizeof(int);
}

unsigned char *LvpGetCustomDataAddr(void)
{
    unsigned char *custom_space_write_data_addr = CUSTOM_TO_XIP_ADDR(WRITE_CUSTOM_DATA_ADDR);
    int len = *(int*)(custom_space_write_data_addr);

    if (len == 0) {
        return NULL;
    }

    return custom_space_write_data_addr + sizeof(int);
}