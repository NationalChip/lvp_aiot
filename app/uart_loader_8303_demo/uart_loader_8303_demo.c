/* Voice Signal Preprocess
 * Copyright (C) 2001-2024 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * uart_loader_8303_demo.c
 *
 */

#include <autoconf.h>
#include <decoder.h>
#include <driver/gx_clock/gx_clock_v2.h>
#include <driver/gx_gpio.h>
#include <driver/gx_pmu_ctrl.h>
#include <driver/gx_rtc.h>
#include <driver/gx_uart.h>
#include <lvp_app.h>
#include <lvp_board.h>
#include <lvp_buffer.h>
#include <lvp_context.h>
#include <lvp_system_init.h>

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
#include <lvp_tts_player.h>
#endif

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
#include <kws_uart_report.h>
#endif

#ifdef CONFIG_APP_UART_LOADER_HAS_LED
#include "vc_led.h"
#endif

// 定义后唤醒词唤醒后mcu不会进入休眠，超时之后才会进入休眠，有些客户会有这个需求
// #define DO_NOT_SLEEP_WHEN_WAKING_UP
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
#include <lvp_pmu.h>
#endif

#ifdef CONFIG_ENABLE_PRINT_SEND_RECV_INFO
#include <lvp_uart_loader.h>
#include "8303_sample_bin.h"
#endif

#ifdef CONFIG_GX8003
#define __LOW_FREQUENCE CPU_FREQUENCE_50M
#else
#ifdef CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_2M
#define __LOW_FREQUENCE CPU_FREQUENCE_2M
#elif defined CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_4M
#define __LOW_FREQUENCE CPU_FREQUENCE_4M
#elif defined CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_8M
#define __LOW_FREQUENCE CPU_FREQUENCE_8M
#elif defined CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_12M
#define __LOW_FREQUENCE CPU_FREQUENCE_12M
#elif defined CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_24M
#define __LOW_FREQUENCE CPU_FREQUENCE_24M
#elif defined CONFIG_APP_UART_LOADER_MCU_LOW_FREQUENCY_50M
#define __LOW_FREQUENCE CPU_FREQUENCE_50M
#endif
#endif

#define __TIME_OUT_S CONFIG_APP_UART_LOADER_KWS_TIME_OUT

#define LOG_TAG "[UART_LOADER_APP]"

typedef struct TIMEOUT_STANDBY {
    unsigned long first_time;
    unsigned long next_time;
    unsigned char timeout_flag;
} TIMEOUT_STANDBY;

static TIMEOUT_STANDBY kws_state_time = {0, 0, 1};  // 超时标志位设置为1，初始状态为超时状态
static int kws_state_init_flag = 0;
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
static int lock = 0;
#endif

//=================================================================================================

static int UartLoaderDemoSuspend(void *priv)
{
    printf(LOG_TAG " ---- %s ----\n", __func__);
    BoardSetPowerSavePinMux();
    return 0;
}

static int UartLoaderDemoResume(void *priv)
{
    BoardSetUserPinMux();
    printf(LOG_TAG " ---- %s ----\n", __func__);
    if (kws_state_time.timeout_flag) LvpDynamiciallyAdjustCpuFrequency(__LOW_FREQUENCE);
    printf("*************** cpu frequency is %dHz *************\n",
            gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
    return 0;
}

static int UartLoaderDemoInit(void)
{
    if (!kws_state_init_flag) {
        kws_state_init_flag = 1;
        printf("TimeOut:%d\n", __TIME_OUT_S);

#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
        LvpPmuSuspendLockCreate(&lock);
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
        LvpSetVuiKwsStates(VUI_KWS_VAD_STATE);  // 框架默认为active状态
#endif
        gx_rtc_get_tick(&kws_state_time.first_time);
#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
        LvpKwsUartReportInit();
#endif

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

#ifdef CONFIG_LVP_HAS_UART_LOADER
        LOADER_STATUS status = LvpUartLoader(uart_run_bin, sizeof(uart_run_bin), 5);
        if (status == LOADER_SUCCESS) {
            printf(LOG_TAG "8303 loader ok\n");
        } else {
            printf(LOG_TAG "8303 loader error[%d]\n", status);
        }
#endif
    }

    return 0;
}

// App Event Process
static int UartLoaderDemoEventResponse(APP_EVENT *app_event)
{
    if (app_event->event_id < 100) return 0;

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
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
            LvpPmuSuspendLock(lock);  // 唤醒后上锁
#endif
        }
        printf("*************** cpu frequency is %dHz *************\n",
                gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
    }
    gx_rtc_get_tick(&kws_state_time.first_time);
#endif

#if (defined CONFIG_APP_UART_LOADER_HAS_LED) || (defined CONFIG_LVP_SELF_LEARN_UES_LED_TIP)
    EnableGpioLed();
    KwsLedFlicker(200, 0, 1);
#endif

    printf(LOG_TAG "event_id %d\n", app_event->event_id);

    return 0;
}

// APP Main Loop
static int UartLoaderDemoTaskLoop(void)
{
    // 超时检测
    if (!kws_state_time.timeout_flag) {
        gx_rtc_get_tick(&kws_state_time.next_time);
        if (kws_state_time.next_time - kws_state_time.first_time > __TIME_OUT_S) {
            kws_state_time.timeout_flag = 1;
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
            APP_EVENT time_out_event = {
                .event_id = 0xf,  // 0xf 代表超时退出
                .ctx_index = 0
            };
            LvpTtsPlayerResponse(&time_out_event);
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
            LvpSetVuiKwsStates(VUI_KWS_VAD_STATE);
#endif
            LvpDynamiciallyAdjustCpuFrequency(__LOW_FREQUENCE);
            printf("*****************cpu frequency is %dHz ************\n",
                    gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
            printf("timeout! ---> next_time%ds - first_time%ds = %ds \n", kws_state_time.next_time,
                   kws_state_time.first_time, kws_state_time.next_time - kws_state_time.first_time);
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
            LvpPmuSuspendUnlock(lock);  // 超时后解锁
#endif
        }
    }

    return 0;
}

LVP_APP uart_loader_demo = {
    .app_name = "uart_loader_demo",
    .AppInit = UartLoaderDemoInit,
    .AppEventResponse = UartLoaderDemoEventResponse,
    .AppTaskLoop = UartLoaderDemoTaskLoop,
    .AppSuspend = UartLoaderDemoSuspend,
    .suspend_priv = "UartLoaderDemoSuspend",
    .AppResume = UartLoaderDemoResume,
    .resume_priv = "UartLoaderDemoResume",
};

LVP_REGISTER_APP(uart_loader_demo);
