/* Grus
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * kws_strategy.h:
 *
 */

#ifndef __KWS_STRATEGY_H__
#define __KWS_STRATEGY_H__

#include <lvp_context.h>
#include <lvp_param.h>

typedef struct {
    float ctc_threshold_offset;
    int ctc_context_counter;
} BIONIC;

void KwsStrategyInit(void);
float KwsStrategyGetThresholdOffset(LVP_CONTEXT *context, float threshold);
void KwsStrategyClearThresholdOffset(void);
int KwsStrategyRunBionic(LVP_CONTEXT *context);

#endif /* __KWS_STRATEGY_H__ */
