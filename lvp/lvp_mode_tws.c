/* Grus
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * grus_loop.c:
 *
 */
#include <autoconf.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <csi_core.h>
#include <soc_config.h>

#include <driver/gx_audio_in.h>
#include <driver/gx_pmu_ctrl.h>
#include <driver/gx_watchdog.h>
#include <driver/gx_delay.h>
#include <driver/gx_clock.h>
#include <driver/gx_irq.h>
#include <driver/gx_snpu.h>
#include <driver/gx_timer.h>
#include <driver/gx_gpio.h>
#include <driver/gx_rtc.h>
#include <driver/gx_irq.h>

#include <lvp_context.h>
#include <lvp_buffer.h>

#include "common/lvp_pmu.h"
#include "common/lvp_queue.h"
#include "common/lvp_audio_in.h"
#include "common/lvp_uart_record.h"
#include "common/lvp_system_init.h"
#include "common/lvp_standby_ratio.h"
#include "app_core/lvp_app_core.h"
#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
#include "lvp_voice_player.h"
#endif
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
#include <decoder.h>
#endif

#include "lvp_mode_tws.h"
#include "lvp_mode.h"
#include "lvp_kws.h"

#define LOG_TAG "[LVP_TWS]"

//=================================================================================================
#ifdef CONFIG_LVP_STANDBY_ENABLE
# ifdef CONFIG_LVP_STATE_FVAD_COUNT_DOWN
#  define LVP_STATE_FVAD_COUNT_DOWN         CONFIG_LVP_STATE_FVAD_COUNT_DOWN
# else
#  define LVP_STATE_FVAD_COUNT_DOWN         (18)
# endif

typedef enum {
    LVP_STANDBY_STATE_WAITING,
    LVP_STANDBY_STATE_AVAD,
    LVP_STANDBY_STATE_FVAD,
    LVP_STANDBY_STATE_WAKEUP,
    LVP_STANDBY_STATE_STANDBY,
}LVP_STANDBY_STATE;

static struct {
    LVP_STANDBY_STATE     state;
    int                 count_down;
} s_standby_state;

DRAM0_STAGE2_SRAM_ATTR void _LvpSetStandbyState(LVP_STANDBY_STATE state)
{
    s_standby_state.state = state;
    switch (state) {
        case LVP_STANDBY_STATE_WAITING:
            break;
        case LVP_STANDBY_STATE_AVAD:
            break;
        case LVP_STANDBY_STATE_FVAD:
            s_standby_state.count_down = LVP_STATE_FVAD_COUNT_DOWN;
            break;
        case LVP_STANDBY_STATE_WAKEUP:
            break;
        default:
            break;
    }
}

DRAM0_STAGE2_SRAM_ATTR void _LvpStandbyStateLoop(void){
    if (s_standby_state.state == LVP_STANDBY_STATE_FVAD) {
        s_standby_state.count_down--;
        if (!s_standby_state.count_down) {
            _LvpSetStandbyState(LVP_STANDBY_STATE_STANDBY);
        }
    }
}
#endif

//=================================================================================================

typedef struct {
    unsigned int module_id;
    void * priv;
} MODULE_INFO;
#define KWS_TASK_QUEUE_BUFFER_SIZE 8
static unsigned char s_kws_task_queue_buffer[KWS_TASK_QUEUE_BUFFER_SIZE * sizeof(MODULE_INFO)];
static LVP_QUEUE s_kws_task_queue;

typedef struct {
    void * priv;
} SNPU_TASK_INFO;
#define SNPU_TASK_QUEUE_BUFFER_SIZE 12

//=================================================================================================
DRAM0_STAGE2_SRAM_ATTR static int _LvpAudioInRecordCallback(int ctx_index, void *priv)
{
    if (ctx_index > 0) {
        LVP_CONTEXT *context;
        unsigned int ctx_size;
        LvpGetContext(ctx_index - 1, &context, &ctx_size);
        context->ctx_index = ctx_index - 1;
        // context->kws         = 0; // 挪到 ctc.c 中清0
        // context->vad         = 0;
        context->G_vad         = 0;

        // Get FFTVad
        int vad = LvpAudioInGetDelayedFFTVad();
        static int last_vad = 0;

#ifdef CONFIG_ENABLE_NOISE_JUDGEMENT
        LvpAuidoInQueryEnvNoise(context);
#endif

        if (context->ctx_header->fft_vad_en) {
            LVP_CONTEXT_HEADER *ctx_header = LvpGetContextHeader();
            if (context->ctx_index <= ctx_header->logfbank_frame_num_per_channel / ctx_header->pcm_frame_num_per_context) {
                context->fft_vad = 1;
            } else {
                context->fft_vad = vad;
            }
        } else {
            context->fft_vad = 1;
        }

#ifdef CONFIG_LVP_STANDBY_ENABLE
        _LvpStandbyStateLoop();

        if (context->fft_vad) {
            _LvpSetStandbyState(LVP_STANDBY_STATE_FVAD);
        }
#endif

#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
        if (!(LvpVoicePlayerGetStatus() != PLAYER_STATUS_PLAY &&
              LvpVoicePlayerGetStatus() != PLAYER_STATUS_PREPARE)) {
            LvpAudioInUpdateReadIndex(1);
        } else {
# ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
            LvpKwsRun(context);
# endif
        }
#else
# ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
        LvpKwsRun(context);
# else
        LvpAudioInUpdateReadIndex(1);
# endif
#endif
        if (context->ctx_index%60 == 0 || (last_vad != context->fft_vad)) {
#ifdef CONFIG_LVP_STANDBY_ENABLE
            int standby_ratio = 0;
# ifdef CONFIG_ENABLE_CACULATE_STANDBY_RATIO
            standby_ratio = LvpCountRealTimeStandbyRatio();
# endif
            printf (LOG_TAG"Ctx:%d, Vad:%d, Ns:%d, R:%d\n", context->ctx_index, context->fft_vad, context->env_noise, standby_ratio);
#endif
            if ((last_vad != context->fft_vad)) {
                last_vad = context->fft_vad;
            }
        }

        APP_EVENT plc_event = {
            .event_id = AUDIO_IN_RECORD_DONE_EVENT_ID,
            .ctx_index = context->ctx_index
        };
        LvpTriggerAppEvent(&plc_event);
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
DRAM0_STAGE2_SRAM_ATTR static int _LvpActiveSnpuCallback(int module_id, GX_SNPU_STATE state, void *priv)
{
    MODULE_INFO module_info = {
        .module_id = module_id,
        .priv      = priv
    };
    LvpQueuePut(&s_kws_task_queue, (unsigned char *)&module_info);

    return 0;
}
#endif
//-------------------------------------------------------------------------------------------------

static int _TwsModeInit(LVP_MODE_TYPE mode)
{
    memset (s_kws_task_queue_buffer, 0, KWS_TASK_QUEUE_BUFFER_SIZE * sizeof(MODULE_INFO));
    LvpQueueInit(&s_kws_task_queue, s_kws_task_queue_buffer, KWS_TASK_QUEUE_BUFFER_SIZE * sizeof(MODULE_INFO), sizeof(MODULE_INFO));
#if (defined CONFIG_LVP_ENABLE_CTC_DECODER) || (defined CONFIG_LVP_ENABLE_CTC_GX_DECODER) || (defined CONFIG_LVP_ENABLE_CTC_AND_BEAMSEARCH_DECODER || (defined CONFIG_LVP_ENABLE_BEAMSEARCH_DECODER))
    LvpInitCtcKws();
#elif defined CONFIG_LVP_ENABLE_MAX_DECODER
    LvpInitMaxKws();
#endif

    GX_WAKEUP_SOURCE start_mode = gx_pmu_get_wakeup_source();

#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    LvpKwsInit(_LvpActiveSnpuCallback, start_mode);
#endif

    if ((mode != LVP_MODE_INIT_FLAG) || (start_mode == GX_WAKEUP_SOURCE_COLD || start_mode == GX_WAKEUP_SOURCE_WDT)) {
        if (0 != LvpAudioInInit(_LvpAudioInRecordCallback)) {
            printf(LOG_TAG"LvpAudioInInit Failed\n");
            return -1;
        }
    } else {
        LvpAudioInStandbyToStartup();
    }

#ifdef CONFIG_LVP_STANDBY_ENABLE
    _LvpSetStandbyState(LVP_STANDBY_STATE_FVAD);
#endif
    return 0;
}

DRAM0_STAGE2_SRAM_ATTR static void _TwsModeTick(void)
{
    MODULE_INFO module_info = {0};
    if (LvpQueueGet(&s_kws_task_queue, (unsigned char *)&module_info)) {
        if (module_info.module_id == 0x100) {
#ifdef CONFIG_LVP_HAS_VOICE_PLAYER
            if (LvpVoicePlayerGetStatus() != PLAYER_STATUS_PLAY &&
                LvpVoicePlayerGetStatus() != PLAYER_STATUS_PREPARE) {
# if (defined CONFIG_LVP_ENABLE_CTC_DECODER) || (defined CONFIG_LVP_ENABLE_CTC_GX_DECODER) || (defined CONFIG_LVP_ENABLE_CTC_AND_BEAMSEARCH_DECODER)
                LvpDoKwsScore((LVP_CONTEXT *)module_info.priv);
# endif
# ifdef CONFIG_LVP_ENABLE_USER_DECODER
                LvpDoUserDecoder((LVP_CONTEXT *)module_info.priv);
# endif
            } else {
                ((LVP_CONTEXT *)module_info.priv)->kws = 0; //  clear kws value
            }
#else

# if (defined CONFIG_LVP_ENABLE_CTC_GX_DECODER)
            LvpDoKwsScore((LVP_CONTEXT *)module_info.priv);
# endif
# ifdef CONFIG_LVP_ENABLE_USER_DECODER
            LvpDoUserDecoder((LVP_CONTEXT *)module_info.priv);
# endif

#endif
            LvpAudioInUpdateReadIndex(1);
        }

        if (((LVP_CONTEXT *)module_info.priv)->ctx_index % 60 == 0) {
            printf (LOG_TAG"Ctx:%d\n", ((LVP_CONTEXT *)module_info.priv)->ctx_index);
        }

        if (((LVP_CONTEXT *)module_info.priv)->kws) {
            LvpAudioInResetPcmRWIndex();
            APP_EVENT plc_event = {
                .event_id = ((LVP_CONTEXT *)module_info.priv)->kws,
                .ctx_index = ((LVP_CONTEXT *)module_info.priv)->ctx_index
            };
            LvpTriggerAppEvent(&plc_event);
        }
    }

#ifdef CONFIG_LVP_STANDBY_ENABLE
    if ((s_standby_state.state == LVP_STANDBY_STATE_STANDBY) && !LvpPmuSuspendIsLocked()) {
        LvpPmuSuspend(LRT_GPIO | LRT_AUDIO_IN | LRT_I2C);
    }
#endif

}

static void _TwsModeDone(LVP_MODE_TYPE next_mode)
{
    memset((void*)&s_kws_task_queue, 0, sizeof(s_kws_task_queue));
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    LvpKwsDone();
#endif
    LvpAudioInDone();
    printf(LOG_TAG"Exit TWS mode\n");
}

static int _TwsModeBufferInit(void)
{
#ifndef CONFIG_LVP_USE_BUFFER_V2
    return LvpInitBuffer();
#else
    return LvpInitTwsBuffer();
#endif
}
//-------------------------------------------------------------------------------------------------

const LVP_MODE_INFO lvp_tws_mode_info = {
    .type = LVP_MODE_TWS,
    .buffer_init = _TwsModeBufferInit,
    .init = _TwsModeInit,
    .done = _TwsModeDone,
    .tick = _TwsModeTick,
};
