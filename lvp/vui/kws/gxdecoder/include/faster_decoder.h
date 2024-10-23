#ifndef __FASTER_DECODER_H__
#define __FASTER_DECODER_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"

#include "fst.h"
#include "umap.h"

typedef struct {
    float beam;
    float am_scale;
    int max_active;
    int min_active;
    float beam_delta;
    float hash_ratio;
    int arc_array_size;
    int loglike_dim;
} GXDecoderOptions;

#define FLOATING_TYPE float

#if (FLOATING_TYPE == float)
# define SCALE_VALUE  (1000.f)
#else
# define SCALE_VALUE  (1.f)
#endif

void GxDecoderInit(FST *fst, GXDecoderOptions *op);
void InitDecoding(FST *fst_, unsigned char *buffer);
int AdvanceDecoding(FST *fst_
                    , float *decoder_window1
                    , int t1
                    , float *decoder_window2
                    , int t2
                    , int model_output_len
                    , int len, int start_index, float log_blank_threshold);
int ReachedFinal(FST *fst_, KV_NODE *finals_);
int PrintAllPath(FST *fst_, KV_NODE *finals_, char **label);
int GetBestPath(FST *fst_, int *results, int len,float *graph_cost, float *Acoustic_cost, KV_NODE *finals_);
FLOATING_TYPE ProcessEmitting(FST *fst_, float *loglike);
void ProcessNonemitting(FST *fst_, FLOATING_TYPE cutoff);
int get_tmp_array_max_memory(void);
int get_max_nonemitting_cnt(void);
int get_max_arc_array_cnt(void);
int get_arc_array_size(void);
int get_all_memroy_size(void);
int get_share_memroy_size(void);
void GXDecoderMemoryMonitoring(void);

#pragma GCC diagnostic pop

#endif
