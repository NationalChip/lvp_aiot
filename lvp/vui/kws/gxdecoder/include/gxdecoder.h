#ifndef __GXDECODER_H__
#define __GXDECODER_H__
#include <lvp_context.h>

int LvpGXDocoderInit(LVP_CONTEXT_HEADER *ctx_header);
int LvpDoGXDecoder(LVP_CONTEXT *context
                , float *decoder_window1
                , int t1
                , float *decoder_window2
                , int t2
                , char *ctc_words
                , int ctc_words_size
                , void *xdecoder_tmp_buffer
                , int label_length
                , int start_index
                , char **gxdecoder_words);
char *LvpGetCtcWords(char *ctc_words);
#endif

