/* Voice Signal Preprocess
* Copyright (C) 2001-2024 NationalChip Co., Ltd
* ALL RIGHTS RESERVED!
*
* self_learn_demo.c
*
*/

#include <autoconf.h>
#include <lvp_app.h>
#include <lvp_system_init.h>
#include <lvp_buffer.h>
#include <lvp_context.h>
#include <lvp_board.h>

#include <driver/gx_rtc.h>
#include <driver/gx_pmu_ctrl.h>
#include <driver/gx_gpio.h>
#include <driver/gx_clock/gx_clock_v2.h>
#include <driver/gx_uart.h>

#include <self_learning.h>
#include <decoder.h>

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
# include <lvp_tts_player.h>
#endif

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
# include <kws_uart_report.h>
#endif

#if (defined CONFIG_APP_SLD_APP_VC_HAS_LED) || (defined CONFIG_LVP_SELF_LEARN_UES_LED_TIP)
# include "vc_led.h"
#endif

#define LOG_TAG "[SELF_LEARN_DEMO]"

// #define DO_NOT_SLEEP_WHEN_WAKING_UP     // 定义后唤醒词唤醒后mcu不会进入休眠，超时之后才会进入休眠，有些客户会有这个需求

#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
#include <lvp_pmu.h>
#endif

#ifdef CONFIG_GX8003
# define __LOW_FREQUENCE CPU_FREQUENCE_50M
#else
# ifdef CONFIG_APP_SLD_MCU_LOW_FREQUENCY_2M
#  define __LOW_FREQUENCE CPU_FREQUENCE_2M
# elif defined CONFIG_APP_SLD_MCU_LOW_FREQUENCY_4M
#  define __LOW_FREQUENCE CPU_FREQUENCE_4M
# elif defined CONFIG_APP_SLD_MCU_LOW_FREQUENCY_8M
#  define __LOW_FREQUENCE CPU_FREQUENCE_8M
# elif defined CONFIG_APP_SLD_MCU_LOW_FREQUENCY_12M
#  define __LOW_FREQUENCE CPU_FREQUENCE_12M
# elif defined CONFIG_APP_SLD_MCU_LOW_FREQUENCY_24M
#  define __LOW_FREQUENCE CPU_FREQUENCE_24M
# elif defined CONFIG_APP_SLD_MCU_LOW_FREQUENCY_50M
#  define __LOW_FREQUENCE CPU_FREQUENCE_50M
# endif
#endif

#define __TIME_OUT_S    CONFIG_APP_SLD_APP_VC_KWS_TIME_OUT

typedef struct TIMEOUT_STANDBY{
    unsigned long first_time;
    unsigned long next_time;
    unsigned char timeout_flag;
}TIMEOUT_STANDBY;

static TIMEOUT_STANDBY kws_state_time = {0, 0, 1};  // 超时标志位设置为1，初始状态为超时状态
static int kws_state_init_flag = 0;
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
static int lock = 0;
#endif

//=================================================================================================

static int SelfLearnDemoSuspend(void *priv)
{
    printf(LOG_TAG" ---- %s ----\n", __func__);
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerSuspend(0);
#endif
    BoardSetPowerSavePinMux();
    return 0;
}

static int SelfLearnDemoResume(void *priv)
{
    BoardSetUserPinMux();
    printf(LOG_TAG" ---- %s ----\n", __func__);
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerResume();
#endif
    if (kws_state_time.timeout_flag)
        LvpDynamiciallyAdjustCpuFrequency(__LOW_FREQUENCE);
    printf("*************** cpu frequency is %dHz *************\n",gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
    return 0;
}

static int SelfLearnDemoInit(void)
{
    if (!kws_state_init_flag) {
        kws_state_init_flag = 1;
        printf("TimeOut:%d\n", CONFIG_APP_SLD_APP_VC_KWS_TIME_OUT);

#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
        LvpPmuSuspendLockCreate(&lock);
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
        LvpSetVuiKwsStates(VUI_KWS_VAD_STATE);   // 框架默认为active状态
#endif
        gx_rtc_get_tick(&kws_state_time.first_time);
#if defined(CONFIG_LVP_HAS_TTS_PLAYER) || defined(CONFIG_LVP_HAS_KWS_UART_REPORT)
        APP_EVENT power_on_event = {
            .event_id = 0x0,  // 0x0 代表开机播报
            .ctx_index = 0
        };
# ifdef CONFIG_LVP_HAS_TTS_PLAYER
        LvpTtsPlayerInit();
# endif
# ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
        LvpKwsUartReportInit();
# endif
        GX_WAKEUP_SOURCE start_mode = gx_pmu_get_wakeup_source();
        if (start_mode == GX_WAKEUP_SOURCE_COLD) {
# ifdef CONFIG_LVP_HAS_TTS_PLAYER
            LvpTtsPlayerResponse(&power_on_event);
# endif
# ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
            LvpKwsUartReportResponse(&power_on_event);
# endif
        }
#endif
    }

    return 0;
}

// App Event Process
static int SelfLearnDemoEventResponse(APP_EVENT *app_event)
{
#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
    if (app_event->event_id == AUDIO_IN_RECORD_DONE_EVENT_ID)
        return 0;

    gx_rtc_get_tick(&kws_state_time.first_time);    // 有事件触发需要刷新计时
    printf("[APP_EVEND_ID]app event_id: %d\n", app_event->event_id);

    switch (app_event->event_id)
    {
    case EVENT_START_LEARN_INSTRUCTION_WORD_KV:     // 连续学习指令词
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
        LvpTtsPlayerResponse(app_event);            // 播报开始连续学习指令词
#endif
        LvpSetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_START, 1);
        break;
    case EVENT_CLEAR_LEARN_INSTRUCTION_WORD_KV:     // 将指令词全部清除
        LvpDeleteInstructionLearnRecord();
        break;
    case EVENT_RESET_LEARN_KV:                      // 重置学习
        LvpLearningReset();
        break;
    case EVENT_START_LEARN_WORD0_KV:                // 学习单个指定词
    case EVENT_START_LEARN_WORD1_KV:
    case EVENT_START_LEARN_WORD2_KV:
    case EVENT_START_LEARN_WORD3_KV:
    case EVENT_START_LEARN_WORD4_KV:
    case EVENT_START_LEARN_WORD5_KV:
    case EVENT_START_LEARN_WORD6_KV:
        LvpLearnsIndividualWord(app_event->event_id);
        break;
    case EVENT_DEL_WORD0_KV:                        // 删除单个指定词
    case EVENT_DEL_WORD1_KV:
    case EVENT_DEL_WORD2_KV:
    case EVENT_DEL_WORD3_KV:
    case EVENT_DEL_WORD4_KV:
    case EVENT_DEL_WORD5_KV:
    case EVENT_DEL_WORD6_KV:
        LvpDeleteIndividualWord(app_event->event_id);
    case EVENT_PLAY_LEARN_COMPLETE_KV:              // 学习完成
        break;
    default:
        break;
    }
#endif

    if (app_event->event_id < 100)
        return 0;

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerResponse(app_event);
#endif

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
    LvpKwsUartReportResponse(app_event);
#endif

#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    LVP_KWS_PARAM *kws_info = LvpGetKwsInfo(app_event->event_id);

    // 没有找到对应的指令词
    if (kws_info == NULL) return 0;

    // 找到主唤醒词
    if ((kws_info->major & 0x1) == 1) {
        if (kws_state_time.timeout_flag) {
            kws_state_time.timeout_flag = 0;
            LvpSetVuiKwsStates(VUI_KWS_ACTIVE_STATE);
            LvpDynamiciallyAdjustCpuFrequency(CPU_FREQUENCE_DEFAULT);
# ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
            LvpPmuSuspendLock(lock);    // 唤醒后上锁
# endif
        }
        printf("*************** cpu frequency is %dHz *************\n",gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
    }
#endif

#if (defined CONFIG_APP_SLD_APP_VC_HAS_LED) || (defined CONFIG_LVP_SELF_LEARN_UES_LED_TIP)
    EnableGpioLed();
    KwsLedFlicker(200, 0, 1);
#endif

    printf(LOG_TAG"event_id %d\n", app_event->event_id);

    return 0;
}


// APP Main Loop
static int SelfLearnDemoTaskLoop(void)
{
    // 超时检测
    if (LvpGetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS)) { // 正在学习状态需要刷新重新计时
        gx_rtc_get_tick(&kws_state_time.first_time);
    }

    if (!kws_state_time.timeout_flag) {
        gx_rtc_get_tick(&kws_state_time.next_time);
        if (kws_state_time.next_time - kws_state_time.first_time > __TIME_OUT_S) {
            kws_state_time.timeout_flag = 1;
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
            APP_EVENT time_out_event = {
                .event_id = 0xf, // 0xf 代表超时退出
                .ctx_index = 0
            };
            LvpTtsPlayerResponse(&time_out_event);
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
            LvpSetVuiKwsStates(VUI_KWS_VAD_STATE);
#endif
            LvpDynamiciallyAdjustCpuFrequency(__LOW_FREQUENCE);
            printf("*****************cpu frequency is %dHz ************\n",gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
            printf("timeout! ---> next_time%ds - first_time%ds = %ds \n", kws_state_time.next_time,\
                    kws_state_time.first_time, kws_state_time.next_time - kws_state_time.first_time);
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
            LvpPmuSuspendUnlock(lock);      // 超时后解锁
#endif
        }
    }

    return 0;
}


LVP_APP self_learn_demo = {
    .app_name = "self_learn_demo",
    .AppInit = SelfLearnDemoInit,
    .AppEventResponse = SelfLearnDemoEventResponse,
    .AppTaskLoop = SelfLearnDemoTaskLoop,
    .AppSuspend = SelfLearnDemoSuspend,
    .suspend_priv = "SelfLearnDemoSuspend",
    .AppResume = SelfLearnDemoResume,
    .resume_priv = "SelfLearnDemoResume",
};

LVP_REGISTER_APP(self_learn_demo);

