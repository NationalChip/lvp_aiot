/* LVP
 * Copyright (C) 2001-2024 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * kws_uart_report.h
 *
 */

#ifndef __KWS_UART_REPORT_H__
#define __KWS_UART_REPORT_H__

#include <autoconf.h>
#include <base_addr.h>
#include <lvp_app.h>

#ifdef CONFIG_CUSTOM_STORAGE_SPACE
# define KWS_UART_REPORT_FLASH_ADDR  (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH + CONFIG_CUSTOM_STORAGE_SPACE_LEN * 4096)
#else
# define KWS_UART_REPORT_FLASH_ADDR  (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH)
#endif

typedef struct {
    unsigned int    event_id;
    unsigned char   report_cmd[CONFIG_KWS_UART_REPORT_CMD_DATA_LEN];
} __attribute__((packed)) KWS_UART_REPORT_INFO;

typedef struct {
    unsigned int list_num;
    KWS_UART_REPORT_INFO *kws_uart_report_info;
} KWS_UART_REPORT_LIST_INFO;

int LvpGetKwsUartReportInfoLength(void);
int LvpKwsUartReportInit(void);
int LvpKwsUartReportSuspend(int type);
int LvpKwsUartReportResume(void);
int LvpKwsUartReportDone(void);
int LvpKwsUartReportResponse(APP_EVENT *app_event);

#endif /* __KWS_UART_REPORT_H__ */
