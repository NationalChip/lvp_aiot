#include <string.h>
#include <driver/gx_uart.h>
#include <driver/gx_snpu.h>
#include <driver/gx_clock/gx_clock_v2.h>
#include <driver/gx_timer.h>

#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
# include <lvp_voice_player.h>
#endif

#include "app_core/lvp_app_core.h"
#include "lvp_uart_tts_player.h"

#if (defined CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL) && (CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL != 0)
# define TIME_OUT_MS    CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL   // 相同指令接收间隔时间，此时间段内接收到相同指令只处理第一次的指令
typedef struct TIMEOUT_UART_PLAY {
    unsigned int  first_time;
    unsigned int  next_time;
    unsigned char timeout_flag;
    int           last_event_id;
} TIMEOUT_UART_PLAY;

static TIMEOUT_UART_PLAY uart_play_time = {0, 0, 1, -1};
#endif

static UART_LIST_INFO g_uart_list_info;
static unsigned char *uart_info_addr = NULL;
static uint8_t uart_recv_flag = 0;
static uint8_t uart_tts_player_buf[REPLY_PLAYER_UART_CMD_DATA_LEN];
static int s_uart_reply_init_flag = 0;

static int ReadUartTtsInfoFromFlash(void)
{
    LvpTtsPlayerInit();
    uart_info_addr = (unsigned char *)(LvpGetTtsInfoStartAddr() + LvpGetTtsInfoLength());

    g_uart_list_info.list_num  = *(int*)uart_info_addr;
    g_uart_list_info.uart_info = (UART_INFO*)(uart_info_addr + sizeof(int));

    printf("[UART INFO][uart_data_num]: %d [cmd_len]: %d\n", g_uart_list_info.list_num, REPLY_PLAYER_UART_CMD_DATA_LEN);
    for (int i = 0; i < g_uart_list_info.list_num; i++) {
        printf("uart_data: ");
        for (int j = 0; j < REPLY_PLAYER_UART_CMD_DATA_LEN; j++) {
            printf("%#x ", g_uart_list_info.uart_info[i].uart_data[j]);
        }
        printf("[event_id]%d\n", g_uart_list_info.uart_info[i].event_id);
    }

    return 0;
}

static int LvpTtsGetCmdEventId(uint8_t *serial_data)
{
    for (int i = 0; i < g_uart_list_info.list_num; i++) {
        if (memcmp(serial_data, g_uart_list_info.uart_info[i].uart_data, REPLY_PLAYER_UART_CMD_DATA_LEN) == 0) {
            int event_id = g_uart_list_info.uart_info[i].event_id;
            printf("event_id: %d\n", event_id);

            return event_id;
        }
    }

    return -1;
}

static int uart_tts_player_recv_callback(int port, int length, void *priv)
{
    memset(uart_tts_player_buf, 0, sizeof(uart_tts_player_buf));
    if (REPLY_PLAYER_UART_CMD_DATA_LEN == gx_uart_read_non_block(port, uart_tts_player_buf, REPLY_PLAYER_UART_CMD_DATA_LEN, 20)) {
        uart_recv_flag = 1;
    }

    return 0;
}

#ifdef CONFIG_PLAYER_UART_NO_INTERRUPT_MODE
static _GetPlyerStatus(void)
{
    int ret = 0;

# ifdef CONFIG_LVP_HAS_VOICE_PLAYER
        if (LvpVoicePlayerGetStatus() != PLAYER_STATUS_PLAY && LvpVoicePlayerGetStatus() != PLAYER_STATUS_PREPARE) {
            ret = 1;
        }
# endif

    return ret;
}
#endif

int LvpGetUartTtsInfoLength(void)
{
    return sizeof(int) + g_uart_list_info.list_num * sizeof(UART_INFO);
}

int LvpUartTtsPlayerInit(void)
{
    if (gx_uart_init(REPLY_PLAYER_UART, REPLY_PLAYER_UART_BAUDRATE) != 0) {
        printf("uart tts player init fail!\n");
        return -1;
    }
    gx_uart_start_async_recv(REPLY_PLAYER_UART, uart_tts_player_recv_callback, NULL);

    if (s_uart_reply_init_flag == 1)
        return 0;
    s_uart_reply_init_flag = 1;
    ReadUartTtsInfoFromFlash();

    return 0;
}

int LvpUartTtsPlayerTick(void)
{
#if (defined CONFIG_LVP_HAS_VOICE_PLAYER)

# ifdef CONFIG_PLAYER_UART_NO_INTERRUPT_MODE    // 是否允许播报的时候进行打断
    if (_GetPlyerStatus()) {
# endif
#if (defined CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL) && (CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL != 0)
        if (!uart_play_time.timeout_flag) {
            uart_play_time.next_time = gx_get_time_ms();
            if ((uart_play_time.next_time - uart_play_time.first_time) >= TIME_OUT_MS) {
                uart_play_time.timeout_flag = 1;
            }
        }
# endif

        if (uart_recv_flag == 1) {
            uart_recv_flag = 0;
            while (gx_snpu_get_state() == GX_SNPU_BUSY);
            int event_id = LvpTtsGetCmdEventId(uart_tts_player_buf);

#if (defined CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL) && (CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL != 0)
            if ((uart_play_time.timeout_flag == 1) || (event_id != uart_play_time.last_event_id)) {
# endif
                APP_EVENT plc_event = {
                    .event_id = event_id,
                    .ctx_index = 0
                };
                LvpTriggerAppEvent(&plc_event);
#if (defined CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL) && (CONFIG_PLAYER_UART_SAME_EVENT_TRIGGER_INTERVAL != 0)
                uart_play_time.timeout_flag = 0;
                uart_play_time.last_event_id = event_id;
                uart_play_time.first_time = gx_get_time_ms();
            }
# endif

            return event_id;
        }
# ifdef CONFIG_PLAYER_UART_NO_INTERRUPT_MODE
    }
# endif

#endif

    return 0;
}

int LvpUartTtsPlayerDone(void)
{
    return 0;
}

int LvpUartTtsPlayerSuspend(int type)
{
    return 0;
}

int LvpUartTtsPlayerResume(void)
{
    return 0;
}

