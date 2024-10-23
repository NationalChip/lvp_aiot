/* Voice Signal Preprocess
* Copyright (C) 2001-2023 NationalChip Co., Ltd
* ALL RIGHTS RESERVED!
*
* ctc_model.c: Kws Model
*
*/

#include <autoconf.h>
#include <stdio.h>
#include <string.h>
#include <lvp_buffer.h>

#include <driver/gx_snpu.h>
#include <driver/gx_cache.h>

#include "ctc_model.h"
#include "model.h"
#include "model_xip.h"

#define LOG_TAG "[KWS]"

__attribute__((unused))  static GX_SNPU_TASK s_snpu_task;

static int use_xip_model_flag = 0;

int LvpModelGetCmdSize(void)
{
   if (use_xip_model_flag) return sizeof(xip_kws_cmd_content);
   else return sizeof(kws_cmd_content);
}

int LvpModelGetWeightSize(void)
{
   if (use_xip_model_flag) return sizeof(xip_kws_weight_content);
   else return sizeof(kws_weight_content);
}

int LvpModelGetOpsSize(void)
{
   if (use_xip_model_flag) return sizeof(xip_kws_ops_content);
   else return sizeof(kws_ops_content);
}

int LvpModelGetDataSize(void)
{
   if (use_xip_model_flag) return sizeof(xip_kws_data_content);
   else return sizeof(kws_data_content);
}

int LvpModelGetTmpSize(void)
{
   if (use_xip_model_flag) return sizeof(xip_kws_tmp_content);
   else return sizeof(kws_tmp_content);
}

void LvpSetSnpuTask(GX_SNPU_TASK *snpu_task)
{
#ifdef CONFIG_NPU_RUN_IN_FLASH
    s_snpu_task.cmd       = (void *)MCU_TO_DEV((unsigned int)kws_cmd_content);
    s_snpu_task.weight    = (void *)MCU_TO_DEV((unsigned int)kws_weight_content);
    s_snpu_task.ops       = (void *)MCU_TO_DEV((unsigned int)(void *)(NPU_SRAM_ADDR/4*4));
    void *data = (void *)(((unsigned int)s_snpu_task.ops + LvpModelGetOpsSize() + 3)/4*4);
    s_snpu_task.data      = (void *)MCU_TO_DEV((unsigned int)data);
    s_snpu_task.tmp_mem   = (void *)MCU_TO_DEV((unsigned int)kws_tmp_content);
#else
    memcpy(&s_snpu_task, snpu_task, sizeof(GX_SNPU_TASK));
#endif
}
void LvpSetSnpuRunInFlash(void)
{
   use_xip_model_flag = 1;
   unsigned int cmd_size = LvpModelGetCmdSize();
   memcpy((void *)(0x20030000+7*1024), xip_kws_cmd_content, cmd_size);
   s_snpu_task.cmd       = (void *)(0x20030000+7*1024);
   gx_dcache_clean_range((unsigned int *)s_snpu_task.cmd, cmd_size);

   s_snpu_task.weight    = (void *)DEV_TO_MCU((unsigned int)xip_kws_weight_content);
   s_snpu_task.ops       = (void *)DEV_TO_MCU((unsigned int)(void *)(NPU_SRAM_ADDR/4*4));
   void *data = (void *)(((unsigned int)s_snpu_task.ops + LvpModelGetOpsSize() + 3)/4*4);
   s_snpu_task.data      = (void *)DEV_TO_MCU((unsigned int)data);
   unsigned int tmp_size = LvpModelGetTmpSize();
   unsigned char *tmp    = (void *)(NPU_SRAM_ADDR/4*4 + (CONFIG_NPU_SRAM_SIZE_KB - 21)*1024);
   memcpy(tmp, xip_kws_tmp_content, tmp_size);
   s_snpu_task.tmp_mem   = (void *)DEV_TO_MCU((unsigned int)tmp);

#if 0
    printf ("\ncmd:\n");
    unsigned char *cmd = (unsigned char *)s_snpu_task.cmd;
    for (int i = 0; i < 10; i++) {
        printf ("%x, ", cmd[i]);
    }
    printf ("\n");
    for (int i = LvpModelGetCmdSize() - 10; i < LvpModelGetCmdSize(); i++) {
        printf ("%x, ", cmd[i]);
    }
    printf ("\n");

    printf ("weight:\n");
    unsigned char *weight = (unsigned char *)s_snpu_task.weight;
    for (int i = 0; i < 10; i++) {
        printf ("%x, ", weight[i]);
    }
    printf ("\n");
    for (int i = LvpModelGetWeightSize() - 10; i < LvpModelGetWeightSize(); i++) {
        printf ("%x, ", weight[i]);
    }
    printf ("\n");

    printf ("tmp:\n");
    unsigned char *tmp_mem = (unsigned char *)s_snpu_task.tmp_mem;
    for (int i = 0; i < 10; i++) {
        printf ("%x, ", tmp_mem[i]);
    }
    printf ("\n");
    for (int i = LvpModelGetTmpSize() - 10; i < LvpModelGetTmpSize(); i++) {
        printf ("%x, ", tmp_mem[i]);
    }
    printf ("\n");
#endif

}

void LvpSetSnpuRunInSram(void)
{
    use_xip_model_flag = 0;
    unsigned int cmd_size = LvpModelGetCmdSize();
    unsigned int weight_size = LvpModelGetWeightSize();

    s_snpu_task.ops  = (void *)(NPU_SRAM_ADDR/4*4);
    s_snpu_task.data = (void *)((unsigned int)s_snpu_task.ops + (LvpModelGetOpsSize() + 3)/4*4);
    s_snpu_task.tmp_mem = (void *)((unsigned int)s_snpu_task.data + (LvpModelGetDataSize() + 3)/4*4);
    s_snpu_task.cmd     = (void *)((unsigned int)s_snpu_task.tmp_mem +(LvpModelGetTmpSize() +3)/4*4);
    s_snpu_task.weight  = (void *)((unsigned int)s_snpu_task.cmd + (cmd_size + 3)/4*4);

    memcpy(s_snpu_task.cmd, kws_cmd_content, cmd_size);
    memcpy(s_snpu_task.weight, kws_weight_content, weight_size);
    gx_dcache_clean_range((unsigned int *)s_snpu_task.cmd, cmd_size);
    gx_dcache_clean_range((unsigned int *)s_snpu_task.weight, weight_size);
}

int LvpCTCModelInitSnpuTask(GX_SNPU_TASK *snpu_task)
{
    if (CONFIG_KWS_SNPU_BUFFER_SIZE < sizeof(struct in_out)) {
        printf ("snpu buffer size is need set:%d\n", sizeof(struct in_out));
        return -1;
    }

    snpu_task->module_id = 0x100;
    snpu_task->cmd       = (void *)MCU_TO_DEV(s_snpu_task.cmd);
    snpu_task->weight    = (void *)MCU_TO_DEV(s_snpu_task.weight);
    snpu_task->ops       = (void *)MCU_TO_DEV(s_snpu_task.ops);
    snpu_task->data      = (void *)MCU_TO_DEV(s_snpu_task.data);
    snpu_task->tmp_mem   = (void *)MCU_TO_DEV(s_snpu_task.tmp_mem);

#if 0
    if (use_xip_model_flag) {
        printf ("\ncmd:\n");
        unsigned char *cmd = (unsigned char *)s_snpu_task.cmd;
        for (int i = 0; i < 10; i++) {
            printf ("%x, ", cmd[i]);
        }
        printf ("\n");
        for (int i = LvpModelGetCmdSize() - 10; i < LvpModelGetCmdSize(); i++) {
            printf ("%x, ", cmd[i]);
        }
        printf ("\n");

        printf ("weight:\n");
        unsigned char *weight = (unsigned char *)s_snpu_task.weight;
        for (int i = 0; i < 10; i++) {
            printf ("%x, ", weight[i]);
        }
        printf ("\n");
        for (int i = LvpModelGetWeightSize() - 10; i < LvpModelGetWeightSize(); i++) {
            printf ("%x, ", weight[i]);
        }
        printf ("\n");

        printf ("tmp:\n");
        unsigned char *tmp_mem = (unsigned char *)s_snpu_task.tmp_mem;
        for (int i = 0; i < 10; i++) {
            printf ("%x, ", tmp_mem[i]);
        }
        printf ("\n");
        for (int i = LvpModelGetTmpSize() - 10; i < LvpModelGetTmpSize(); i++) {
            printf ("%x, ", tmp_mem[i]);
        }
        printf ("\n");
    }
#endif
    return 0;
}

const char *LvpCTCModelGetKwsVersion(void)
{
    return kws_version;
}

void *LvpCTCModelGetSnpuOutBuffer(void *snpu_buffer)
{
    if (use_xip_model_flag) return (void *)((struct in_out_xip*)snpu_buffer)->Model_rnn_out;
    else return (void *)((struct in_out*)snpu_buffer)->Model_rnn_out;
}

void *LvpCTCModelGetSnpuFeatsBuffer(void *snpu_buffer)
{
    if (use_xip_model_flag) return (void *)((struct in_out_xip*)snpu_buffer)->Feats;
    else return (void *)((struct in_out*)snpu_buffer)->Feats;
}

void *LvpCTCModelGetSnpuStateBuffer(void *snpu_buffer)
{
    if (use_xip_model_flag) return (void *)((struct in_out_xip*)snpu_buffer)->State_c0;
    else return (void *)((struct in_out*)snpu_buffer)->State_c0;
}
unsigned int LvpCTCModelGetSnpuFeatsDim(void)
{
    return CONFIG_KWS_MODEL_INPUT_WIN_LENGTH * CONFIG_KWS_MODEL_FEATURES_DIM_PER_FRAME;
}

unsigned int LvpCTCModelGetSnpuStateDim(void)
{
    if (use_xip_model_flag) return sizeof(struct input_xip)/sizeof(short) - LvpCTCModelGetSnpuFeatsDim();
    else return sizeof(struct input)/sizeof(short) - LvpCTCModelGetSnpuFeatsDim();
}
