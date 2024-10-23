/* Voice Signal Preprocess
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * lvp_app_event.c:
 */

#include <autoconf.h>

#include <stdio.h>
#include <string.h>

#include <lvp_system_init.h>
#include <lvp_buffer.h>
#include <board_config.h>
#include <driver/gx_timer.h>

#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
#include <lvp_voice_player.h>
#endif

#ifdef CONFIG_LVP_HAS_WATCHDOG
#include <driver/gx_watchdog.h>
#endif

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
#include <lvp_uart_tts_player.h>
#endif

#include "lvp_uart_record.h"
#include "lvp_pmu.h"
#include "lvp_queue.h"
#include "lvp_app_core.h"
#include "lvp_app.h"
#include "uart_message_v2.h"

#define LOG_TAG "[APP_EVENT]"

__attribute__((weak)) LVP_APP *app_core_ops = NULL;
//=================================================================================================

#define LVP_APP_MISC_QUEUE_LEN 8
static unsigned char s_app_misc_event_queue_buffer[LVP_APP_MISC_QUEUE_LEN * sizeof(APP_EVENT)] = {0};
LVP_QUEUE s_app_misc_event_queue;

#if ((defined CONFIG_LVP_ENABLE_G_SENSOR_VAD) || (defined CONFIG_UART_RECORD_ENABLE))
#define LVP_APP_ENABLE_AIN_CB_EVENT
#endif
#ifdef LVP_APP_ENABLE_AIN_CB_EVENT
#define LVP_APP_AIN_CB_QUEUE_LEN 3
static unsigned char s_app_ain_cb_event_queue_buffer[LVP_APP_AIN_CB_QUEUE_LEN * sizeof(APP_EVENT)] = {0};
LVP_QUEUE s_app_ain_cb_event_queue;
#endif

//=================================================================================================
int LvpTriggerAppEvent(APP_EVENT *app_event)
{
#ifdef CONFIG_LVP_ENABLE_G_SENSOR_VAD
    unsigned int kws_id = app_event->event_id;
    if ((kws_id >= 100 && kws_id <= 255) && !LvpGetGvad()) return 0;  // Gvad未激活时,过滤掉kws事件
#endif

    LvpQueuePut(&s_app_misc_event_queue, (const unsigned char *)app_event);

#ifdef LVP_APP_ENABLE_AIN_CB_EVENT
    if (app_event->event_id == AUDIO_IN_RECORD_DONE_EVENT_ID) {
        LvpQueuePut(&s_app_ain_cb_event_queue, (const unsigned char *)app_event);
    }
#endif

    return 0;
}

static int _LvpAppSuspend(void *priv)
{
#ifdef CONFIG_LVP_HAS_WATCHDOG
    gx_watchdog_stop();
#endif
    if ((app_core_ops) && (app_core_ops->AppSuspend)) {
        app_core_ops->AppSuspend(app_core_ops->suspend_priv);
    }
    return 0;
}

static int _LvpAppResume(void *priv)
{
    if((app_core_ops) && (app_core_ops->AppResume)) {
        app_core_ops->AppResume(app_core_ops->resume_priv);
    }
    return 0;
}

#ifdef CONFIG_LVP_HAS_WATCHDOG
static int _WatchdogCallback(int irq, void *pdata)
{
    gx_reboot();
    return 0;
}
#endif

int LvpInitializeAppEvent(void)
{
#ifdef CONFIG_UART_RECORD_ENABLE
    UartRecordInit(-1, 0);
#endif

    LvpQueueInit(&s_app_misc_event_queue, s_app_misc_event_queue_buffer, LVP_APP_MISC_QUEUE_LEN * sizeof(APP_EVENT), sizeof(APP_EVENT));
#ifdef LVP_APP_ENABLE_AIN_CB_EVENT
    LvpQueueInit(&s_app_ain_cb_event_queue, s_app_ain_cb_event_queue_buffer, LVP_APP_AIN_CB_QUEUE_LEN * sizeof(APP_EVENT), sizeof(APP_EVENT));
#endif

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
    LvpUartTtsPlayerInit();
#endif

    if((app_core_ops) && (app_core_ops->AppInit))
        app_core_ops->AppInit();

    LVP_SUSPEND_INFO suspend_info = {
        .suspend_callback = _LvpAppSuspend,
        .priv = "_LvpAppSuspend"
    };

    LVP_RESUME_INFO resume_info = {
        .resume_callback = _LvpAppResume,
        .priv = "_LvpAppResume"
    };
    LvpSuspendInfoRegist(&suspend_info);
    LvpResumeInfoRegist(&resume_info);

#ifdef CONFIG_LVP_HAS_WATCHDOG
    gx_watchdog_init(CONFIG_LVP_HAS_WATCHDOG_RESET_MS ,\
                        CONFIG_LVP_HAS_WATCHDOG_TIMEOUT_MS,\
                        _WatchdogCallback, NULL);
#endif
    return 0;
}

int LvpAppEventTick(void)
{
#ifdef CONFIG_UART_RECORD_ENABLE
    UartRecordTick();
#endif

#ifdef CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER
    LvpUartTtsPlayerTick();
#endif

    APP_EVENT app_event = {.event_id = 0};

    if (LvpQueueGet(&s_app_misc_event_queue, (unsigned char *)&app_event)) {
#ifdef CONFIG_LVP_HAS_WATCHDOG_TICK_BLOCK
        gx_watchdog_ping(); // 确保 ain 中断正常才喂狗
#endif
        if((app_core_ops) && (app_core_ops->AppEventResponse))
            app_core_ops->AppEventResponse(&app_event);
    }

#ifdef LVP_APP_ENABLE_AIN_CB_EVENT
    if (LvpQueueGet(&s_app_ain_cb_event_queue, (unsigned char *)&app_event)) {
# ifdef CONFIG_UART_RECORD_ENABLE
        LVP_CONTEXT *context;
        unsigned int ctx_size;
        LvpGetContext(app_event.ctx_index, &context, &ctx_size);
        UartRecordTask(context);
# endif
    }
#endif

    if((app_core_ops) && (app_core_ops->AppTaskLoop)) {
        app_core_ops->AppTaskLoop();
    }
#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
    LvpVoicePlayerTask(NULL);
#endif

#ifdef CONFIG_LVP_HAS_UART_MESSAGE_2_0
    UartMessageAsyncTick();
#endif

    return 0;
}


