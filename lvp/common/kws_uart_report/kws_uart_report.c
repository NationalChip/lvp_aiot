#include <string.h>
#include <driver/gx_uart.h>
#include <driver/gx_snpu.h>
#include <driver/gx_clock/gx_clock_v2.h>
#include <driver/gx_timer.h>

#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
# include <self_learning.h>
#endif

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
# include "uart_tts_play/lvp_uart_tts_player.h"
#endif

#include "app_core/lvp_app_core.h"
#include "kws_uart_report.h"

#define LOG_TAG "[KWS_UART_REPORT]"
#define TO_XIP_ADDR(addr)   (unsigned char*)(CONFIG_FLASH_XIP_BASE + (unsigned int)addr)

static unsigned char g_kws_uart_report_init_flag = 0;
static unsigned char *kws_uart_report_info_addr = TO_XIP_ADDR(KWS_UART_REPORT_FLASH_ADDR);
static KWS_UART_REPORT_LIST_INFO g_kws_uart_report_info;

#if (CONFIG_KWS_UART_REPORT_CMD_TIMES > 1)
static int send_timer_id = 0;

static int _kws_uart_report_timer_cb(void* send_cmd)
{
    static int cnt = 0;
    gx_uart_send_buffer(CONFIG_KWS_UART_REPORT_UART, (unsigned char *)send_cmd, CONFIG_KWS_UART_REPORT_CMD_DATA_LEN);
    cnt ++;
    if (cnt >= (CONFIG_KWS_UART_REPORT_CMD_TIMES - 1)) {
        cnt = 0;
        gx_timer_unregister(send_timer_id);
    }

    return 0;
}
#endif

static int _ReadKwsUartReportInfoFromFlash(void)
{
#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
    kws_uart_report_info_addr += LvpGetSelfLearnInfoLength();
#endif
    g_kws_uart_report_info.list_num = *(int*)kws_uart_report_info_addr;
    g_kws_uart_report_info.kws_uart_report_info = (KWS_UART_REPORT_INFO*)(kws_uart_report_info_addr + sizeof(int));

    printf(LOG_TAG "[list_num]: %d [report_cmd_len]%d\n", g_kws_uart_report_info.list_num, CONFIG_KWS_UART_REPORT_CMD_DATA_LEN);
    for (int i = 0; i < g_kws_uart_report_info.list_num; i++) {
        printf("event_id[%d], report_cmd:[", g_kws_uart_report_info.kws_uart_report_info[i].event_id);
        for (int j = 0; j < CONFIG_KWS_UART_REPORT_CMD_DATA_LEN; j++) {
            printf("%#x ", g_kws_uart_report_info.kws_uart_report_info[i].report_cmd[j]);
        }
        printf("]\n");
    }

    return 0;
}

int LvpKwsUartReportResponse(APP_EVENT *app_event)
{
    for (int i = 0; i < g_kws_uart_report_info.list_num; i++) {
        if (g_kws_uart_report_info.kws_uart_report_info[i].event_id == app_event->event_id) {
            gx_uart_send_buffer(CONFIG_KWS_UART_REPORT_UART
                                , (unsigned char *)g_kws_uart_report_info.kws_uart_report_info[i].report_cmd
                                , CONFIG_KWS_UART_REPORT_CMD_DATA_LEN);
#if (CONFIG_KWS_UART_REPORT_CMD_TIMES > 1)
            send_timer_id = gx_timer_register(_kws_uart_report_timer_cb
                                            , CONFIG_KWS_UART_REPORT_CMD_INTERVAL
                                            , g_kws_uart_report_info.kws_uart_report_info[i].report_cmd
                                            , GX_TIMER_MODE_CONTINUE);
#endif
        }
    }

    return 0;
}

int LvpGetKwsUartReportInfoLength(void)
{
    return sizeof(int) + g_kws_uart_report_info.list_num * sizeof(KWS_UART_REPORT_INFO);
}

int LvpKwsUartReportInit(void)
{
#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
# if (CONFIG_KWS_UART_REPORT_UART == CONFIG_REPLY_PLAYER_UART)
#   if (CONFIG_KWS_UART_REPORT_UART_BAUDRATE != CONFIG_REPLY_PLAYER_UART_BAUDRATE) // 串口使用一致，但波特率不一致，报错提醒
#    error "[ERROR] Two functions are using the same UART port simultaneously, but they have different baud rate settings"
#   endif
# else   // 串口使用的不一致
    if (gx_uart_init(CONFIG_KWS_UART_REPORT_UART, CONFIG_KWS_UART_REPORT_UART_BAUDRATE) != 0) {
        printf("uart tts player init fail!\n");
        return -1;
    }
# endif
#else   // 未开启 UART_TTS_REPLY_PLAYER
    if (gx_uart_init(CONFIG_KWS_UART_REPORT_UART, CONFIG_KWS_UART_REPORT_UART_BAUDRATE) != 0) {
        printf("uart tts player init fail!\n");
        return -1;
    }
#endif

    if (g_kws_uart_report_init_flag == 1)
        return 0;
    g_kws_uart_report_init_flag = 1;
    _ReadKwsUartReportInfoFromFlash();

    return 0;
}

int LvpKwsUartReportDone(void)
{
    return 0;
}

int LvpKwsUartReportSuspend(int type)
{
    return 0;
}

int LvpKwsUartReportResume(void)
{
    return 0;
}

