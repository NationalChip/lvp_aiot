/* Voice Signal Preprocess
* Copyright (C) 2001-2024 NationalChip Co., Ltd
* ALL RIGHTS RESERVED!
*
* lvp_app_kws_state_demo.c
*
*/

#include <lvp_app.h>
#include <lvp_system_init.h>
#include <lvp_buffer.h>
#include <lvp_context.h>
#include <lvp_board.h>

#include <driver/gx_rtc.h>
#include <driver/gx_gpio.h>
#include <driver/gx_clock/gx_clock_v2.h>
#include <driver/gx_pmu_ctrl.h>
#include <driver/gx_uart.h>

#include <decoder.h>

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
# include <lvp_tts_player.h>
#endif

#ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
# include <kws_uart_report.h>
#endif

#ifdef CONFIG_APP_VC_HAS_LED
# include "vc_led.h"
#endif

#ifdef CONFIG_GX_STANDARD_SERIAL_UART_PROTOCOL
# include "vc_message.h"
#endif

#ifdef CONFIG_USER_STANDARD_SERIAL_UART_PROTOCOL
# include <string.h>

# define BUF_SIZE 1024
# define MESSAGE_UART CONFIG_APP_USE_UART
# define MESSAGE_UART_BAUDRATE CONFIG_APP_USE_UART_BAUDRATE

static uint8_t uart_recv_flag = 0;
static uint8_t uart_buffer[BUF_SIZE];
#endif

#define LOG_TAG "[KWS_STATE_DEMO_APP]"

// #define DO_NOT_SLEEP_WHEN_WAKING_UP     // 定义后唤醒词唤醒后mcu不会进入休眠，超时之后才会进入休眠，有些客户会有这个需求

#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
#include <lvp_pmu.h>
#endif

#ifdef CONFIG_GX8003
# define __LOW_FREQUENCE CPU_FREQUENCE_50M
#else
# ifdef CONFIG_MCU_LOW_FREQUENCY_2M
#  define __LOW_FREQUENCE CPU_FREQUENCE_2M
# elif defined CONFIG_MCU_LOW_FREQUENCY_4M
#  define __LOW_FREQUENCE CPU_FREQUENCE_4M
# elif defined CONFIG_MCU_LOW_FREQUENCY_8M
#  define __LOW_FREQUENCE CPU_FREQUENCE_8M
# elif defined CONFIG_MCU_LOW_FREQUENCY_12M
#  define __LOW_FREQUENCE CPU_FREQUENCE_12M
# elif defined CONFIG_MCU_LOW_FREQUENCY_24M
#  define __LOW_FREQUENCE CPU_FREQUENCE_24M
# elif defined CONFIG_MCU_LOW_FREQUENCY_50M
#  define __LOW_FREQUENCE CPU_FREQUENCE_50M
# endif
#endif

#define __TIME_OUT_S    CONFIG_APP_VC_KWS_TIME_OUT

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

static int KwsStateDemoAppSuspend(void *priv)
{
    printf(LOG_TAG" ---- %s ----\n", __func__);
#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerSuspend(0);
#endif
    BoardSetPowerSavePinMux();
    return 0;
}

static int KwsStateDemoAppResume(void *priv)
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

static int KwsStateDemoAppInit(void)
{
    if (!kws_state_init_flag)
    {
#ifdef CONFIG_GX_STANDARD_SERIAL_UART_PROTOCOL
        VCMessageInit();
#endif
#ifdef CONFIG_USER_STANDARD_SERIAL_UART_PROTOCOL
        gx_uart_init(MESSAGE_UART, MESSAGE_UART_BAUDRATE); // 串口初始化函数例子
#endif
        printf("TimeOut:%d\n", CONFIG_APP_VC_KWS_TIME_OUT);
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
        LvpPmuSuspendLockCreate(&lock);
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
        LvpSetVuiKwsStates(VUI_KWS_VAD_STATE);   // 框架默认为active状态
#endif
        kws_state_init_flag = 1;
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
static int KwsStateDemoAppEventResponse(APP_EVENT *app_event)
{
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

    LVP_CONTEXT *context;
    unsigned int ctx_size;
    LvpGetContext(app_event->ctx_index, &context, &ctx_size);

    // 找到主唤醒词
    if ((kws_info->major & 0x1) == 1) {
        if (kws_state_time.timeout_flag) {
            kws_state_time.timeout_flag = 0;
            LvpSetVuiKwsStates(VUI_KWS_ACTIVE_STATE);
            LvpDynamiciallyAdjustCpuFrequency(CPU_FREQUENCE_DEFAULT);
#ifdef DO_NOT_SLEEP_WHEN_WAKING_UP
            LvpPmuSuspendLock(lock);    // 唤醒后上锁
#endif
        }
        printf("*************** cpu frequency is %dHz *************\n",gx_clock_get_module_frequence(CLOCK_MODULE_SCPU));
    }
    gx_rtc_get_tick(&kws_state_time.first_time);
#endif

#ifdef CONFIG_APP_VC_HAS_LED
    EnableGpioLed();
    KwsLedFlicker(200, 0, 1);
#endif

    printf(LOG_TAG"event_id %d\n", app_event->event_id);

#ifdef CONFIG_GX_STANDARD_SERIAL_UART_PROTOCOL
    VCNewMessageNotify(app_event->event_id);
#endif

#ifdef CONFIG_USER_STANDARD_SERIAL_UART_PROTOCOL
    uart_buffer[0] = 0x1;
    uart_buffer[1] = 0x2;
    uart_buffer[2] = 0x3;
    uart_buffer[3] = 0x4;
    memcpy(uart_buffer + 4, &(app_event->event_id), sizeof(app_event->event_id));
    gx_uart_send_buffer(MESSAGE_UART, (unsigned char *)uart_buffer, 4 + sizeof(app_event->event_id)); // 串口自定义发送例子
#endif
    return 0;
}

#ifdef CONFIG_USER_STANDARD_SERIAL_UART_PROTOCOL
static int uart_can_receive_callback(int port, int length, void *priv)
{
    memset(uart_buffer, 0, sizeof(uart_buffer));
    gx_uart_recv_buffer(port, uart_buffer, 128);
    gx_uart_stop_async_recv(port);
    uart_recv_flag = 1;
    return 0;
}
#endif

// APP Main Loop
static int KwsStateDemoAppTaskLoop(void)
{
    // 超时检测
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
# ifdef CONFIG_LVP_HAS_KWS_UART_REPORT
            LvpKwsUartReportResponse(&time_out_event);
# endif
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

#ifdef CONFIG_USER_STANDARD_SERIAL_UART_PROTOCOL
    gx_uart_start_async_recv(MESSAGE_UART, uart_can_receive_callback, NULL); // 串口中断接收例子
    if (uart_recv_flag == 1)
    {
        uart_recv_flag = 0;
        printf("buffer length: %d\n", strlen(uart_buffer));
        printf("buffer data is %s\r\n", uart_buffer);
    }
#endif
    return 0;
}


LVP_APP kws_state_demo_app = {
    .app_name = "kws state demo app",
    .AppInit = KwsStateDemoAppInit,
    .AppEventResponse = KwsStateDemoAppEventResponse,
    .AppTaskLoop = KwsStateDemoAppTaskLoop,
    .AppSuspend = KwsStateDemoAppSuspend,
    .suspend_priv = "KwsStateDemoAppSuspend",
    .AppResume = KwsStateDemoAppResume,
    .resume_priv = "KwsStateDemoAppResume",
};

LVP_REGISTER_APP(kws_state_demo_app);

