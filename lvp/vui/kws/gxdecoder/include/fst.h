#ifndef __FST_H__
#define __FST_H__
#include "gxdecoder_conf.h"
#include "umap.h"
#pragma pack(4)
typedef struct {
    unsigned short ilabel:9;
    unsigned short olabel:10;
    unsigned short next_state:13;
    float weight;
} ARC;

int ArcCompare(ARC *a, ARC *b);

typedef struct {
    unsigned short key;
    /*算法中主要使用hash表存float和地址*/
    float f;
} FST_NODE;
typedef struct {
    int start_;
    unsigned short *arc_offset_;
    ARC *arcs_;
    int arcs_cnt;
    int arc_offset_cnt;
    FST_NODE *final_node;
    int final_cnt;
} FST;

typedef struct {
    int start_;
    unsigned short arc_offset_[MAX_STATES];
    ARC arcs_[MAX_ARCS];
    int arcs_cnt;
    int arc_offset_cnt;
    FST_NODE final_node[FINALS_LENGTH];
    int final_cnt;
} FST_BIN;

void FstReset(FST *fst_);
int FstStart(FST *fst_);
void FstSetStart(FST *fst_, int id);
int FstNumFinals(FST *fst_);
int FstNumArcs(FST *fst);
int FstNumStates(FST *fst);
int IsFinal(FST *fst_, KV_NODE *finals_, int id);
float Final(FST *fst_, KV_NODE *finals_, int id);
int FstGetArcStart(FST *fst_, int id);
int FstGetArcEnd(FST *fst_, int id);




#endif

