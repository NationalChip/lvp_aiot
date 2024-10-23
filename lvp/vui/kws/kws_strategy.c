/* Grus
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * ctc_decoder.c: The Process For Kws
 *
 */

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <lvp_context.h>
#include <lvp_param.h>

#include "kws_strategy.h"
#include "decoder.h"

#define LOG_TAG "[ST]"

#ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
static BIONIC s_bionic = { 0 };
#endif

void KwsStrategyInit(void)
{
}

float KwsStrategyGetThresholdOffset(LVP_CONTEXT *context, float threshold)
{
#ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
    return s_bionic.ctc_threshold_offset;
#else
    return 0.f;
#endif
}

void KwsStrategyClearThresholdOffset(void)
{
#ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
    s_bionic.ctc_context_counter = 0;
    s_bionic.ctc_threshold_offset = 0.f;
#endif
}

__attribute__((unused)) int KwsStrategyRunBionic(LVP_CONTEXT *context)
{
#if defined CONFIG_LVP_ENABLE_CTC_BIONIC
    LVP_CONTEXT_HEADER *ctx_header = context->ctx_header;
    int ctx_ms = ctx_header->frame_length * ctx_header->pcm_frame_num_per_context;
    int ctc_time_out = CONFIG_LVP_KWS_THRESHOLD_ADJUST_TIME * 1000;

    s_bionic.ctc_context_counter ++;
    if (s_bionic.ctc_context_counter * ctx_ms > ctc_time_out && s_bionic.ctc_threshold_offset < CONFIG_LVP_KWS_MAX_THRESHOLD_ADJUSTMENT_VALUE) {
        s_bionic.ctc_context_counter = 0;
        if (s_bionic.ctc_threshold_offset + CONFIG_LVP_KWS_THRESHOLD_STEP > CONFIG_LVP_KWS_MAX_THRESHOLD_ADJUSTMENT_VALUE)
            s_bionic.ctc_threshold_offset = CONFIG_LVP_KWS_MAX_THRESHOLD_ADJUSTMENT_VALUE;
        else
            s_bionic.ctc_threshold_offset += CONFIG_LVP_KWS_THRESHOLD_STEP;
        printf(LOG_TAG"ctc threshold_offset: %d\n", (int)(s_bionic.ctc_threshold_offset));
    }
#endif

    return 0;
}
