#include <autoconf.h>
#include <stdio.h>
#include <string.h>
#include <lvp_attr.h>

#include "fst.h"
#include "faster_decoder.h"
#include "umap.h"
#include <math.h>

#include "fst_bin.h"
#include "words_list.h"

// #ifdef CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION
// #include <kws_list.h>
// #endif

#include "decoder.h"
#include <driver/gx_timer.h>

#define LOG_TAG "[LVP_XDECODER]"

#define GXDECODER_INPUT_LENGTH (CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1)
static FST fst;
static char s_gxdecoder_words[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH + 10] DRAM0_AUDIO_IN_ATTR = {0};
static char s_ctc_decoder_words[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH + 10] DRAM0_AUDIO_IN_ATTR= {0};
#ifdef CONFIG_ENABLE_SWITCH_NPU_MODEL_RUN_IN_FLASH_OR_SRAM
static KV_NODE fst_finals[FINALS_LENGTH] DRAM0_AUDIO_IN_ATTR;
#else
static KV_NODE fst_finals[FINALS_LENGTH] DRAM0_AUDIO_IN_ATTR;
#endif
static float s_blank_buffer[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH] DRAM0_AUDIO_IN_ATTR;
static float s_log_blank_threshold = 0.f;
static int result[MAX_RESULTS] DRAM0_AUDIO_IN_ATTR;

GXDecoderOptions op = {
    .am_scale = 1.0f,
    .beam = 8,
    .max_active = 50,
    .min_active = 20,
    .beam_delta = 0.5f,
    .hash_ratio = 2.f,
    .arc_array_size = MAX_ARCS*sizeof(float)/3 > POOL_SIZE*sizeof(float) ? MAX_ARCS*sizeof(float)/3 : POOL_SIZE*sizeof(float),
    .loglike_dim = CONFIG_KWS_MODEL_OUTPUT_LENGTH-1,
};

void DelChar(char *str, char c )
{
  int i,j;
  for(i=j=0;str[i]!='\0';i++)
  {
    if(str[i]!=c)//判断是否有和待删除字符一样的字符
    {
      str[j++]=str[i];
    }
  }
  str[j]='\0';//字符串结束
}

int LvpGXDocoderInit(LVP_CONTEXT_HEADER *ctx_header)
{
    static int first = 1;
    if (first) {
        GxDecoderInit(&fst, &op);
        FST_BIN* tmp_fst_bin = (FST_BIN*)&fst_bin;
        float f_blank_threshold = (float)CONFIG_BLANK_THRESHOLD_FOR_CTC_GX_DECODER/1000.f;
        s_log_blank_threshold = (logf(f_blank_threshold)- 2.0f) * 0.9f;
        printf("UMAP_BUCKET_LEN:%d\n", UMAP_BUCKET_LEN);
        printf("UMAP_TABLE_LEN:%d\n", UMAP_TABLE_LEN);
        printf("final num:%d[%d]\n", FINALS_LENGTH, tmp_fst_bin->final_cnt);
        printf("state num:%d\n", MAX_STATES);
        printf("arc num:%d\n", MAX_ARCS);
        printf("TMP_ARRAY_SIZE:%d\n", TMP_ARRAY_SIZE);
        printf("POOL_SIZE:%d\n", POOL_SIZE);
        printf("TPoolGetSize:%d\n", TPoolGetSize());
        printf("hash_get_size:%d\n", hash_get_size());
        printf("KV_NODE:%d\n", sizeof(KV_NODE));
        printf("finals_buffer_size:%d\n", sizeof(fst_finals));
        printf("blank_threshold:%f, s_log_blank_threshold:%f\n", f_blank_threshold, s_log_blank_threshold);
        printf("## need total memory:%d Byte\n", get_all_memroy_size());
        printf("## share memory:%d Byte\n\n", get_share_memroy_size());

        fst.start_          = tmp_fst_bin->start_;
        fst.arc_offset_     = tmp_fst_bin->arc_offset_;
        fst.arcs_           = tmp_fst_bin->arcs_;
        fst.arcs_cnt        = tmp_fst_bin->arcs_cnt;
        fst.arc_offset_cnt  = tmp_fst_bin->arc_offset_cnt;
        fst.final_node      = tmp_fst_bin->final_node;
        fst.final_cnt       = tmp_fst_bin->final_cnt;

        for (int i = 0; i < fst.final_cnt; i++) {
            fst_finals[i].key = fst.final_node[i].key;
            fst_finals[i].value.f = fst.final_node[i].f;
        }
        first = 0;
    }

    return 0;
}

static void swap(float* a, float* b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

static int partition(float arr[], int low, int high) {
    float pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] >= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }

    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

static float quickSelect(float arr[], int low, int high, int N) {
    if (low < high) {
        int pivotIndex = partition(arr, low, high);

        if (pivotIndex == N - 1) {
            return arr[pivotIndex];
        } else if (pivotIndex > N - 1) {
            return quickSelect(arr, low, pivotIndex - 1, N);
        } else {
            return quickSelect(arr, pivotIndex + 1, high, N);
        }
    }

    return arr[low];
}

float findNthLargest(float arr[], int length, int N) {
    if (N < 1 || N > length) {
        printf("无效的N值\n");
        return 0.0f;
    }

    return quickSelect(arr, 0, length - 1, N);
}

char *LvpGetCtcWords(char *ctc_words)
{
    strcpy(s_ctc_decoder_words, ctc_words);
    DelChar(s_ctc_decoder_words, ' ');
    s_ctc_decoder_words[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH + 10 - 1] = '\0';
    return &s_ctc_decoder_words[0];
}

void LvpGXdecoderMemoryMonitoring(void)
{
    GXDecoderMemoryMonitoring();
}

DRAM0_STAGE2_SRAM_ATTR int LvpDoGXDecoder(LVP_CONTEXT *context
                , float *decoder_window1
                , int t1
                , float *decoder_window2
                , int t2
                , char *ctc_words
                , int ctc_words_size
                , void *xdecoder_tmp_buffer
                , int label_length
                , int start_index
                , char **gxdecoder_words)
{
    // printf("x-index:%d\n" , CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - start_index - 4);
    int act_flg = 0;
    float graph_cost = 0.f;
    float ac_cost = 0.f;

    // FST_BIN* tmp_fst_bin = (FST_BIN*)&fst_bin;
    // fst.start_          = tmp_fst_bin->start_;
    // fst.arc_offset_     = tmp_fst_bin->arc_offset_;
    // fst.arcs_           = tmp_fst_bin->arcs_;
    // fst.arcs_cnt        = tmp_fst_bin->arcs_cnt;
    // fst.arc_offset_cnt  = tmp_fst_bin->arc_offset_cnt;
    // fst.final_node      = tmp_fst_bin->final_node;
    // fst.final_cnt       = tmp_fst_bin->final_cnt;

    unsigned int start_ms = gx_get_time_ms();
    GxDecoderInit(&fst, &op);
    InitDecoding(&fst, (unsigned char *)xdecoder_tmp_buffer);
    unsigned int end_ms = gx_get_time_ms();

    float log_blank_threshold = s_log_blank_threshold;
    int start = CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - start_index - 4;
    if (start < 0) start = 0;

    memset(s_blank_buffer, 0, sizeof(s_blank_buffer));
    int skip_cnt = 0;
    int m = start;
    int len = CONFIG_KWS_MODEL_OUTPUT_LENGTH;
    while(m < t1) {
        float blank = decoder_window1[m*len+len-1];
        blank = 0.9*(logf(blank + (1e-6)) - 2.0f);
        if (blank > s_log_blank_threshold) skip_cnt++;
        s_blank_buffer[m-start] = blank;
        m++;
    }
    while(m < CONFIG_KWS_MODEL_DECODER_WIN_LENGTH) {
        float blank = decoder_window2[(m-t1)*len+len-1];
        blank = 0.9*(logf(blank + (1e-6)) - 2.0f);
        if (blank > s_log_blank_threshold) skip_cnt++;
        s_blank_buffer[m-start] = blank;
        m++;
    }

    if (CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - start - skip_cnt < label_length) {
        int nth = CONFIG_KWS_MODEL_DECODER_WIN_LENGTH-start - (label_length + 1);
        log_blank_threshold =  findNthLargest(s_blank_buffer, CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - start, nth);
        printf("blank_th:%f, %d\n", log_blank_threshold, nth);
    }
    unsigned int end_ms_1 = gx_get_time_ms();
    int decoder_num = AdvanceDecoding(&fst, decoder_window1, t1, decoder_window2, t2, len, CONFIG_KWS_MODEL_DECODER_WIN_LENGTH, start, log_blank_threshold);
    unsigned int end_ms_2 =  gx_get_time_ms();
    int result_cnt = GetBestPath(&fst, result, MAX_RESULTS, &graph_cost, &ac_cost, &fst_finals[0]);
    // unsigned int end_ms_2 = gx_get_time_ms();
    printf("[%d][%d,%d] %d-", end_ms_2 - start_ms, end_ms - start_ms, end_ms_1 - end_ms, decoder_num);

    memset(s_gxdecoder_words, 0, sizeof(s_gxdecoder_words));
    memset(s_ctc_decoder_words, 0, sizeof(s_ctc_decoder_words));
    for (int i = 0; i < result_cnt; i++) {
        strcat(s_gxdecoder_words, gxdecoder_data[result[i]].word);
    }
    // printf("\n");
    // memcpy(ctc_decoder_words, ctc_words, ctc_words_size);
    // ctc_decoder_words[ctc_words_size - 1] = '\0';
    // DelChar(ctc_decoder_words, ' ');
    char *ctc_decoder_words = LvpGetCtcWords(ctc_words);
    if (strstr(s_gxdecoder_words, ctc_decoder_words))
    {
        act_flg = 1;
        ResetCtcWinodw();
    }
    *gxdecoder_words = s_gxdecoder_words;
    printf("%s,%s\n", s_gxdecoder_words, ctc_decoder_words);
    printf("\n");

    return act_flg;
}
