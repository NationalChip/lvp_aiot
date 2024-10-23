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

#include <driver/gx_timer.h>
#include <driver/gx_cache.h>
#include <driver/gx_irq.h>

#include <lvp_system_init.h>
#include <lvp_audio_in.h>
#include <lvp_context.h>
#include <lvp_param.h>
#include "lvp_buffer.h"
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
#include <ctc_model.h>
#include <kws_list.h>
#endif

#include "decoder.h"
#include <unistd.h>
#include <gxdecoder.h>
#include <lvp_pmu.h>
#include "kws_strategy.h"

#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
# include "self_learning.h"
#endif

#ifdef CONFIG_ENABLE_CLOSE_CMD_RECOGNIZE
# include <lvp_custom_space.h>
#endif

#define LOG_TAG "[CTC_GXDECODER]"
#define XDECODE_INPUT_LENGTH (CONFIG_KWS_MODEL_OUTPUT_LENGTH-1)

static int s_ctc_index = 0;
static int s_ctc_decoder_head;
static VUI_KWS_STATE s_state = VUI_KWS_ACTIVE_STATE;
#ifdef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
static float s_ctc_decoder_window[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH][CONFIG_KWS_MODEL_OUTPUT_LENGTH] ALIGNED_ATTR(16);
static void *s_gxdecoder_tmp_buffer = NULL;
#else
static KWS_DELAY2DECODE s_kws_delay2decode[sizeof(g_kws_param_list) / sizeof(LVP_KWS_PARAM)] DRAM0_AUDIO_IN_ATTR;
static float s_ctc_decoder_window[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH][CONFIG_KWS_MODEL_OUTPUT_LENGTH] ALIGNED_ATTR(16);
static unsigned char *s_gxdecoder_tmp_buffer;//[50*1024] ALIGNED_ATTR(16) ;
#endif
static char *s_gxdecoder_words = NULL;

#ifdef CONFIG_ENABLE_FAST_DECODER
typedef struct {
    int cnt;
    int cur_ctx_index;
    int last_ctx_index;
    char last_gxdecoder_words[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH + 10];
} FAST_DECODER;
FAST_DECODER g_fast_decoder;
#endif

#ifdef CONFIG_ENABLE_DO_NOT_SLEEP_WHEN_AT_WAKE_UP
static int lock = 0;
#endif

void *GetCtcDecoderWindow(void)
{
    return (void*)s_ctc_decoder_window;
}

LVP_KWS_PARAM_LIST g_kws_list;

DRAM0_STAGE2_SRAM_ATTR void ResetCtcWinodw(void)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    for (int i = 0; i < CONFIG_KWS_MODEL_DECODER_WIN_LENGTH; i++) {
        for (int j = 0; j < CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1; j++) {
            s_ctc_decoder_window[i][j] = (1 - 0.9999f) / (CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1);
        }
        s_ctc_decoder_window[i][CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1] = 0.9999f;
    }
    s_ctc_decoder_head = CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
    s_ctc_index = 0;

# ifdef CONFIG_ENABLE_FAST_DECODER
    memset(&g_fast_decoder, 0, sizeof(FAST_DECODER));
# endif
#endif
}

static void ResetDelay2Decode(void)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
# ifndef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
    memset (s_kws_delay2decode, 0, sizeof(s_kws_delay2decode));
# endif
#endif
}

void LvpInitCtcKws(void)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    g_kws_list.count = sizeof(g_kws_param_list) / sizeof(LVP_KWS_PARAM);
    g_kws_list.kws_param_list = g_kws_param_list;
    ResetCtcWinodw();

    LVP_CONTEXT_HEADER *ctx_header = (LVP_CONTEXT_HEADER *)LvpGetContextHeader();
    LvpGXDocoderInit(ctx_header);
    ResetDelay2Decode();

    KwsStrategyInit();
# ifdef CONFIG_ENABLE_DO_NOT_SLEEP_WHEN_AT_WAKE_UP
    LvpPmuSuspendLockCreate(&lock);
# endif
# ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
    LvpSelfLearnInit();
# endif
#endif
}

void LvpPrintCtcKwsList(void)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
# ifdef CONFIG_ENABLE_USED_FOR_ALL_GRUS_FAMILY
    printf("G:%d\n", 1);
# endif
# ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
    printf(LOG_TAG"BIONIC:%d,%d,%d\n", CONFIG_LVP_KWS_THRESHOLD_STEP, CONFIG_LVP_KWS_MAX_THRESHOLD_ADJUSTMENT_VALUE, CONFIG_LVP_KWS_THRESHOLD_ADJUST_TIME);
# endif
    printf(LOG_TAG"Kws Version: [%s]\n", LvpCTCModelGetKwsVersion());

    g_kws_list.count = sizeof(g_kws_param_list) / sizeof(LVP_KWS_PARAM);
    g_kws_list.kws_param_list = g_kws_param_list;
    printf (LOG_TAG"Demo Kws List [Total:%d]:\n", g_kws_list.count);
    for (int i = 0; i < g_kws_list.count; i++) {
        printf(LOG_TAG"KWS: %s | ", g_kws_list.kws_param_list[i].kws_words);
        printf("KV: %02d | ", g_kws_list.kws_param_list[i].kws_value);
        printf("TRH: %02d | ", g_kws_list.kws_param_list[i].threshold);
        printf("Major: %d | ", g_kws_list.kws_param_list[i].major);
        printf("L: [");
        for (int j = 0; j < g_kws_list.kws_param_list[i].label_length; j++) {
            printf("%d ", g_kws_list.kws_param_list[i].labels[j]);
        }
        printf("]\n");
    }
#endif
}

DRAM0_STAGE2_SRAM_ATTR LVP_KWS_PARAM_LIST *LvpGetKwsParamList(void)
{
    return &g_kws_list;
}

DRAM0_STAGE2_SRAM_ATTR int PrepareData(float *rnn_out)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION

    int idx = s_ctc_index % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
    memcpy(&s_ctc_decoder_window[idx], rnn_out, CONFIG_KWS_MODEL_OUTPUT_LENGTH * sizeof(float));
    s_ctc_index++;

    return CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
#else
    return 0;
#endif
}


DRAM0_STAGE2_SRAM_ATTR static int _LvpDoGroupScore(LVP_CONTEXT *context, int index, int group_number, int valid_frame_num, int major)
{

#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
    static int skip_frame_flag = 0;
    int kws_run_gxdecoder_flag  = 0;
# ifdef ENABLE_NEW_GROUPING_METHOD
    int group_num       = sizeof(grout_offset_index) / sizeof(grout_offset_index[0]) - 1;
    if (skip_frame_flag && (major == 0) && (group_num == 1))
# else
    if (skip_frame_flag && (major == 0) && (CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH == 1))
# endif
    {
        skip_frame_flag = 0;
        return 0;
    }
    for (int i = index; i < group_number; i++) {
#ifdef CONFIG_ENABLE_CLOSE_CMD_RECOGNIZE
        if (LvpGetCloseStatus(g_kws_list.kws_param_list[i].kws_value)) {
            continue;
        }
#endif
        if (valid_frame_num < sizeof(g_kws_list.kws_param_list[i].label_length)) continue;
        if (s_gxdecoder_words!=NULL) {
            char *ctc_decoder_words = LvpGetCtcWords(g_kws_list.kws_param_list[i].kws_words);
            if (strstr(s_gxdecoder_words , ctc_decoder_words)) {
                if((g_kws_list.kws_param_list[i].major&0xf) != major) {
                    ResetCtcWinodw(); // 存在包含关系，且一个为指令词，一个为免唤醒词(主唤醒词)，就直接清窗
                }
            } else {
                continue;
            }
        }
        if (((g_kws_list.kws_param_list[i].major&0xf) == major) || (major == 0)) {
            unsigned short *labels = NULL;
            int label_length = 0;
            labels = (unsigned short *)&g_kws_list.kws_param_list[i].labels[0];
            label_length = g_kws_list.kws_param_list[i].label_length;

# ifdef CONFIG_LVP_DISABLE_XIP_WHILE_CODE_RUN_AT_SRAM
            LvpXipSuspend();
# endif
            int idx = s_ctc_index % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
            int idx0 = idx;
            if (CONFIG_KWS_MODEL_INPUT_STRIDE_LENGTH == 6) {
                if (g_kws_list.kws_param_list[i].label_length <= 8) {
                    if (CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - 4 * g_kws_list.kws_param_list[i].label_length > 0) {
                        idx0 = (s_ctc_index + CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - 4 * g_kws_list.kws_param_list[i].label_length) % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
                        if (idx > idx0) {
                            idx = 0;
                        }
                    }
                }
            }
            int start_index = 0;
            float ctc_score = LvpFastCtcBlockScorePlus(&s_ctc_decoder_window[idx0][0]
                            , valid_frame_num - idx0// - 1
                            , &s_ctc_decoder_window[0][0]
                            , idx// - 1
                            , CONFIG_KWS_MODEL_OUTPUT_LENGTH
                            , CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1
                            , labels
                            , label_length
                            , &start_index);

# ifdef CONFIG_LVP_DISABLE_XIP_WHILE_CODE_RUN_AT_SRAM
            LvpXipResume();
# endif

            float threshold = ((float)(g_kws_list.kws_param_list[i].threshold)/10.f);
            float score = ctc_score;
            // printf("score:%f\n", score);
# ifdef CONFIG_LVP_ENABLE_ONLY_PRINTF_SCORE_WITHOUT_ACTIVATE
            printf (LOG_TAG"ctx_id:%d, Kws:%s[%d], score:%d\n"
                        , context->ctx_index
                        , g_kws_list.kws_param_list[i].kws_words
                        , g_kws_list.kws_param_list[i].kws_value
                        , (int)(10*score));
            continue;
# else
# if 0
            if (score > threshold-5 && score < 120.f) {
                printf (LOG_TAG"ctx_id:%d, Kws:%s[%d], score:%d\n"
                        , context->ctx_index
                        , g_kws_list.kws_param_list[i].kws_words
                        , g_kws_list.kws_param_list[i].kws_value
                        , (int)(10*score));
            }
# endif

# ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
            if (((g_kws_list.kws_param_list[i].major&0xf) == 1)
                || ((g_kws_list.kws_param_list[i].major&0xf) == 2)) {
                    if (KwsStrategyGetThresholdOffset(context, threshold) > 0.f) kws_run_gxdecoder_flag = 1;
            }
# endif

            if ((score > threshold) && (score < 100.f)) {
# ifndef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
                int delay_2_decode_num = (g_kws_list.kws_param_list[i].major & 0xf0000) >> 16;
                if ((s_kws_delay2decode[i].cnt < delay_2_decode_num)) {
                    s_kws_delay2decode[i].cnt++;
                    printf (LOG_TAG"ctx:%d,%d\n", context->ctx_index, s_kws_delay2decode[i].cnt);
                    continue;
                }
# endif
                if (((g_kws_list.kws_param_list[i].major&0xf) == 1)&&(kws_run_gxdecoder_flag == 0)) {
                    context->kws = g_kws_list.kws_param_list[i].kws_value;
                    printf (LOG_TAG" Activation ctx:%d,Kws:%s[%d],th:%d,S:%d,%d\n"
                            , context->ctx_index
                            , g_kws_list.kws_param_list[i].kws_words
                            , g_kws_list.kws_param_list[i].kws_value
                            , g_kws_list.kws_param_list[i].threshold
                            , (int)(10*score)
                            , (int)(10*threshold));
                    ResetCtcWinodw();
                    KwsStrategyClearThresholdOffset();
                    ResetDelay2Decode();
# ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
                    LvpLearnResetDelay();
# endif
                    return 0;
                } else {
                    printf (LOG_TAG" ctc activation ctx:%d,Kws:%s[%d],th:%d,S:%d,%d\n"
                        , context->ctx_index
                        , g_kws_list.kws_param_list[i].kws_words
                        , g_kws_list.kws_param_list[i].kws_value
                        , g_kws_list.kws_param_list[i].threshold
                        , (int)(10*score)
                        , (int)(10*threshold));
                    LvpAudioInSuspend();
                    int idx = s_ctc_index % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
                    int ret = 1;
                    if (s_gxdecoder_words == NULL) {
                        // while (gx_snpu_get_state() == GX_SNPU_BUSY);
                        gx_snpu_pause();
                        ret = LvpDoGXDecoder(context
                                            , &s_ctc_decoder_window[idx][0]
                                            , CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - idx// - 1
                                            , &s_ctc_decoder_window[0][0]
                                            , idx// - 1
                                            , g_kws_list.kws_param_list[i].kws_words
                                            , sizeof(g_kws_list.kws_param_list[i].kws_words)
                                            , (void *)s_gxdecoder_tmp_buffer
                                            , g_kws_list.kws_param_list[i].label_length
                                            , start_index
                                            , &s_gxdecoder_words);
                        gx_dcache_clean_range((unsigned int*)NPU_SRAM_ADDR, CONFIG_NPU_SRAM_SIZE_KB * 1024);
                        gx_snpu_resume();
                    }
                    LvpAudioInResume();
                    if(ret) {
                        ResetDelay2Decode();
# ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
                        LvpLearnResetDelay();
# endif
                        ResetCtcWinodw();
                        KwsStrategyClearThresholdOffset();
# ifdef CONFIG_ENABLE_PRINTF_GXDECODER_MEMORY_MONITORING
                        static int cnt = 0;
                        if ((cnt++)%10 == 0) LvpGXdecoderMemoryMonitoring();
# endif
                        context->kws = g_kws_list.kws_param_list[i].kws_value;
                        printf (LOG_TAG" Activation ctx:%d,Kws:%s[%d],th:%d,S:%d,%d\n"
                                , context->ctx_index
                                , g_kws_list.kws_param_list[i].kws_words
                                , g_kws_list.kws_param_list[i].kws_value
                                , g_kws_list.kws_param_list[i].threshold
                                , (int)(10*score)
                                , (int)(10*threshold));
                    } else {
                        skip_frame_flag = 1;

# ifdef CONFIG_ENABLE_FAST_DECODER
                        if (g_fast_decoder.cnt > 1) {
                            if (context->ctx_index - g_fast_decoder.last_ctx_index > CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH) {
                                memset (&g_fast_decoder, 0, sizeof(FAST_DECODER));
                            }
                        }
                        if (strstr(s_gxdecoder_words, g_fast_decoder.last_gxdecoder_words)) {
                            g_fast_decoder.last_ctx_index  = context->ctx_index;
                            g_fast_decoder.cnt ++;
                        } else {
                            g_fast_decoder.cnt = 0;
                            memcpy(g_fast_decoder.last_gxdecoder_words, s_gxdecoder_words, sizeof(g_fast_decoder.last_gxdecoder_words));
                        }
#ifdef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
                        if (g_fast_decoder.cnt > CONFIG_FAST_DECODER_SKIP_INVALID_CONTEXT) {
#else
                        if (g_fast_decoder.cnt + s_kws_delay2decode[i].cnt > CONFIG_FAST_DECODER_SKIP_INVALID_CONTEXT) {
#endif
                            printf (LOG_TAG"Drop it[%d]\n", g_fast_decoder.cnt);
                            g_fast_decoder.cnt = 0;
                            ResetCtcWinodw();
                        }
# endif
                    }
                    // ResetCtcWinodw();

                }
            } else {
                if (score > ((float)(g_kws_list.kws_param_list[i].threshold)/10.f)) {
                    KwsStrategyClearThresholdOffset();
                }
            }
#endif
        }
    }
    return -1;
#else
    return 0;
#endif
}

int LvpSetVuiKwsStates(VUI_KWS_STATE state)
{
    s_state = state;

#ifdef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
    LvpAudioInSuspend();
    while (gx_snpu_get_state() == GX_SNPU_BUSY);
    gx_disable_irq();
    if (s_state == VUI_KWS_ACTIVE_STATE) {
        printf(LOG_TAG"Switch NPU Run In Flash\n");
        LvpSetSnpuRunInFlash();
        s_gxdecoder_tmp_buffer = (void *)(NPU_SRAM_ADDR/4*4  + (LvpModelGetDataSize() + 3)/4*4);
        memset ((void *)NPU_SRAM_ADDR, 0, (LvpModelGetDataSize() + 3)/4*4);
        gx_dcache_clean_range((unsigned int *)NPU_SRAM_ADDR, (LvpModelGetDataSize() + 3)/4*4);
        printf(LOG_TAG"Switch Success\n");
    } else {
        printf(LOG_TAG"Switch NPU Run In Sram\n");
        LvpSetSnpuRunInSram();
        gx_dcache_clean_range((unsigned int *)NPU_SRAM_ADDR, (LvpModelGetDataSize() + 3)/4*4);
        printf(LOG_TAG"Switch Success\n");
    }
    gx_enable_irq();
    LvpAudioInResume();
#else
    s_gxdecoder_tmp_buffer = (unsigned char *)LvpGetAudioOutBuffer();
    printf(LOG_TAG"s_gxdecoder_tmp_buffer:%x\n", s_gxdecoder_tmp_buffer);
#endif

#ifdef CONFIG_ENABLE_DO_NOT_SLEEP_WHEN_AT_WAKE_UP
    if (VUI_KWS_ACTIVE_STATE == state) {
        LvpPmuSuspendLock(lock);
    } else {
        LvpPmuSuspendUnlock(lock);
    }
#endif
    return 0;
}

int LvpGetVuiKwsStates(void)
{
    return s_state;
}

LVP_KWS_PARAM *LvpGetKwsInfo(int kws_kv)
{
    for (int i = 0; i < g_kws_list.count; i++) {
        if (g_kws_list.kws_param_list[i].kws_value == kws_kv) {
            return &(g_kws_list.kws_param_list[i]);
        }
    }
    return NULL;
}

#ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
static int LvpDoSelfLearn(LVP_CONTEXT *context, int valid_frame_num)
{
    // 开始学习
    if ((LvpGetSlefLearnStates(FLAG_START_SINGLE_LEARNING)) ||
        (LvpGetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_START))) {

        if (!LvpGetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS)) {
            LvpLearningStart(context);
        } else {
            _LvpDoGroupScore(context, 0, g_kws_list.count, valid_frame_num, 4);// 退出学习
            if (context->kws == EVENT_EXIT_SELF_LEARN_KV) {
                LvpLearningExit(context);
                return 2;
            }
            LvpLearningIn(context, &s_ctc_decoder_window[0][0], s_ctc_index);
        }

        return 1;
    }

    // 有学习记录
    if ((LvpGetSlefLearnStates(FLAG_SINGLE_LEARNING_RECORD) != 0) ||
        (LvpGetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_RECORD) != 0)) {
        LvpDoSelfLearnScore(context, &s_ctc_decoder_window[0][0], s_ctc_index, 2);
        if (s_state == VUI_KWS_VAD_STATE) {
            LvpDoSelfLearnScore(context, &s_ctc_decoder_window[0][0], s_ctc_index, 1);
        } else if (s_state == VUI_KWS_ACTIVE_STATE) {
            LvpDoSelfLearnScore(context, &s_ctc_decoder_window[0][0], s_ctc_index, 0);
        }
    }

    return 0;
}
#endif

DRAM0_STAGE2_SRAM_ATTR int LvpDoKwsScore(LVP_CONTEXT *context)
{
#ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION

    float *rnn_out;
    unsigned char *snpu_buffer = LvpGetSnpuBuffer(context, context->ctx_index);
    gx_dcache_invalid_range((unsigned int *)snpu_buffer, context->ctx_header->snpu_buffer_size/context->ctx_header->snpu_buffer_num);
    rnn_out = (float *)LvpCTCModelGetSnpuOutBuffer(snpu_buffer);

    context->kws = 0;// 挪到此处的原因是这段代码执行时间比较久,主要是 softmax
    s_gxdecoder_words = NULL;

# ifdef CONFIG_ENABLE_CTC_SOFTMAX_CYCLE_STATISTIC
    int start_softmax_ms = gx_get_time_ms();
# endif
    int valid_frame_num = PrepareData(rnn_out);
# ifdef CONFIG_ENABLE_CTC_SOFTMAX_CYCLE_STATISTIC
    int end_softmax_ms = gx_get_time_ms();
    printf ("softmax:%d ms\n", end_softmax_ms - start_softmax_ms);
# endif

    if (context->ctx_index < LvpGetLogfbankFrameNumPerChannel() / LvpGetPcmFrameNumPerContext() + LvpGetAudioInCtrlStartCtxIndex()) return 0;

# ifdef CONFIG_LVP_SELF_LEARNING_ENABLE
    if (LvpDoSelfLearn(context, valid_frame_num) != 0) {
        return 0;
    }
# endif

# ifdef ENABLE_NEW_GROUPING_METHOD
    int group_num       = sizeof(grout_offset_index) / sizeof(grout_offset_index[0]) - 1;
    int mod             = context->ctx_index % group_num;
    int grout_offset    = grout_offset_index[mod];
    int grout_count     = grout_offset_index[mod + 1];

    // printf("mod: %d, group_num: %d, grout_offset: %d, grout_count: %d\n", mod, group_num, grout_offset, grout_count);
# else
    int grout_count  = g_kws_list.count;
    int mod          = context->ctx_index % CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH;
    int grout_offset = grout_count * mod / CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH;

    if ((mod + 1) < CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH) {
        grout_count  = (grout_count * mod + grout_count) / CONFIG_KWS_MODEL_DECODER_STRIDE_LENGTH;
    }
# endif

# ifdef CONFIG_ENABLE_CTC_DECODER_CYCLE_STATISTIC
    int start_ms = gx_get_time_ms();
# endif
    int ret = 0;
    if (s_state == VUI_KWS_VAD_STATE) {
# ifdef CONFIG_LVP_ENABLE_CTC_BIONIC
        KwsStrategyRunBionic(context);
# endif
        // Detect Major Kws
        ret |= _LvpDoGroupScore(context, 0, g_kws_list.count, valid_frame_num, 1);//主唤醒词走CTC
        ret |= _LvpDoGroupScore(context, 0, g_kws_list.count, valid_frame_num, 3);//主唤醒词走gxdecoder
        ret |= _LvpDoGroupScore(context, 0, g_kws_list.count, valid_frame_num, 2);//免唤醒词
    }else if (s_state == VUI_KWS_ACTIVE_STATE) {
        // Detect all Kws
        _LvpDoGroupScore(context, grout_offset, grout_count, valid_frame_num, 0);//all kws
    }
# ifdef CONFIG_ENABLE_CTC_DECODER_CYCLE_STATISTIC
    int end_ms = gx_get_time_ms();
    printf ("decoder:%d ms\n", end_ms - start_ms);
# endif

#endif

    return 0;
}
