
#include <stdio.h>
#include <driver/gx_timer.h>
#include <driver/gx_snpu.h>
#include <lvp_buffer.h>
#include <lvp_pmu.h>
#include <driver/gx_pmu_ctrl.h>

#ifdef CONFIG_USE_OPUS
# include <lvp_voice_player.h>
#endif

#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
# include <self_learning.h>
#endif

#include "lvp_tts_player.h"

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
# include "uart_tts_play/lvp_uart_tts_player.h"
#endif

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
# include "kws_uart_report/kws_uart_report.h"
#endif

static unsigned char *flash_addr = TO_XIP_ADDR(TTS_BIN_FLASH_ADDR);
static TTS_LIST_INFO g_tts_list_info;
static int uart_info_list_num = 0;
static int s_tts_play_init_flag = 0;
#ifdef CONFIG_LVP_STANDBY_ENABLE
static int pmu_lock = 0;
#endif

static int ReadTTSInfoFromFlash(void)
{
#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
    LvpKwsUartReportInit();
    flash_addr += LvpGetKwsUartReportInfoLength();
    printf("flash_addr+uart_report: %#x, uart_report:%#x\n", flash_addr, LvpGetKwsUartReportInfoLength());
#endif

#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
    LvpSelfLearnInit();
    flash_addr += LvpGetSelfLearnInfoLength();
    printf("flash_addr+self_learn: %#x, self_learn:%#x\n", flash_addr, LvpGetSelfLearnInfoLength());
#endif

    g_tts_list_info.list_num = *(int*)flash_addr;
    g_tts_list_info.tts_info = (TTS_INFO*)(flash_addr + sizeof(int));
    uart_info_list_num       = *(int*)(flash_addr + sizeof(int) + g_tts_list_info.list_num * sizeof(TTS_INFO));

    printf("[TTS INFO][list_num]: %d\n", g_tts_list_info.list_num);
    for (int i = 0; i < g_tts_list_info.list_num; i++) {
        printf("[kws_id]%d, [tts_num]%d, [offset]%d, [length]%d\n", \
                g_tts_list_info.tts_info[i].kws_id, g_tts_list_info.tts_info[i].tts_num, \
                g_tts_list_info.tts_info[i].offset, g_tts_list_info.tts_info[i].length);
    }

    return 0;
}

static int GenerateRandomNumber(int start, int end)
{
    int random_num = (gx_get_time_ms() % (end - start + 1)) + start;

    return random_num;
}

static int ReadTTSResouceInfo(int kws_id, unsigned char **play_addr, unsigned int *play_len)
{
    for (int i = 0; i < g_tts_list_info.list_num; i++) {
        if (g_tts_list_info.tts_info[i].kws_id == kws_id) {
            int random = 0;
            if (g_tts_list_info.tts_info[i].tts_num != 1) {
                random = GenerateRandomNumber(0, g_tts_list_info.tts_info[i].tts_num - 1);
            }

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
            *play_addr = (unsigned char *)(flash_addr + sizeof(int) + uart_info_list_num * sizeof(UART_INFO) + g_tts_list_info.tts_info[i + random].offset);
#else
            *play_addr = (unsigned char *)(flash_addr + g_tts_list_info.tts_info[i + random].offset);
#endif
            *play_len  = g_tts_list_info.tts_info[i + random].length;

            return 0;
        }
    }

    return -1;
}

static void PlayerEventCallback(int player_event_id, void *data)
{
#ifdef CONFIG_LVP_STANDBY_ENABLE
    LvpPmuSuspendUnlock(pmu_lock);
#endif
#if ((defined CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM) && (defined CONFIG_LVP_HAS_TTS_PLAYER))
    extern void ResetCtcWinodw(void);
    ResetCtcWinodw();
#endif
}

static void TtsPlay(const unsigned char *resource_position, unsigned int length, unsigned int repeat)
{
#ifdef CONFIG_LVP_STANDBY_ENABLE
    LvpPmuSuspendLock(pmu_lock);
#endif

#ifdef CONFIG_USE_OPUS
    LvpVoicePlayerPlay(resource_position);
#endif
}

int LvpGetTtsInfoStartAddr(void)
{
    return (int)flash_addr;
}

int LvpGetTtsInfoLength(void)
{
    return sizeof(int) + g_tts_list_info.list_num * sizeof(TTS_INFO);
}

int LvpTtsPlayerInit(void)
{
    if (s_tts_play_init_flag == 1)
        return 0;

#ifdef CONFIG_LVP_STANDBY_ENABLE
    LvpPmuSuspendLockCreate(&pmu_lock);
#endif

    s_tts_play_init_flag = 1;
    ReadTTSInfoFromFlash();

#ifdef CONFIG_USE_OPUS
    LvpVoicePlayerInit(PlayerEventCallback);
#endif

    return 0;
}

int LvpTtsPlayerResponse(APP_EVENT *app_event)
{
    unsigned char *play_addr = NULL;
    unsigned int play_len   = 0;

    while (gx_snpu_get_state() == GX_SNPU_BUSY);
    if (0 == ReadTTSResouceInfo(app_event->event_id, &play_addr, &play_len)) {
        TtsPlay(play_addr, play_len, 0);
    }

    return 0;
}

int LvpTtsPlayerDone(void)
{
    return 0;
}

int LvpTtsPlayerSuspend(int type)
{
    LvpVoicePlayerSuspend();
    return 0;
}

int LvpTtsPlayerResume(void)
{
    LvpVoicePlayerResume();
    return 0;
}
