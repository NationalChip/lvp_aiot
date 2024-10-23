#ifndef __SELF_LEARNING_H__
#define __SELF_LEARNING_H__

#include <autoconf.h>
#include <lvp_context.h>
#include <base_addr.h>

typedef enum {
    EVENT_START_LEARN_WORD0_KV = 1,
    EVENT_START_LEARN_WORD1_KV,
    EVENT_START_LEARN_WORD2_KV,
    EVENT_START_LEARN_WORD3_KV,
    EVENT_START_LEARN_WORD4_KV,
    EVENT_START_LEARN_WORD5_KV,
    EVENT_START_LEARN_WORD6_KV,
    EVENT_START_LEARN_WORD7_KV,
    EVENT_START_LEARN_WORD8_KV,
    EVENT_START_LEARN_WORD9_KV,
    EVENT_DEL_WORD0_KV = 20,
    EVENT_DEL_WORD1_KV,
    EVENT_DEL_WORD2_KV,
    EVENT_DEL_WORD3_KV,
    EVENT_DEL_WORD4_KV,
    EVENT_DEL_WORD5_KV,
    EVENT_DEL_WORD6_KV,
    EVENT_DEL_WORD7_KV,
    EVENT_DEL_WORD8_KV,
    EVENT_DEL_WORD9_KV,
    EVENT_LEARN_CONTINUE_KV = 30,                       // 在当前窗长学习到的label长度太短时需要继续学习
    EVENT_START_LEARN_INSTRUCTION_WORD_KV,              // 学习指令词
    EVENT_CLEAR_LEARN_INSTRUCTION_WORD_KV,              // 清除指令词
    EVENT_RESET_LEARN_KV,                               // 重置学习
    EVENT_EXIT_SELF_LEARN_KV,                           // 退出学习
    EVENT_PLAY_LEARN_SUCCES_KV,                         // 学习成功播报 --> 学习成功，请在说一次
    EVENT_PLAY_LEARN_FAIL_KV,                           // 学习失败播报 --> 学习失败，请在说一次
    EVENT_PLAY_LEARN_COMPLETE_KV,                       // 学习完成播报 --> 学习完成
    EVENT_PLAY_LEARN_SAME_KV,                           // 学习一致播报 --> 本次学习与已学习的记录一致，请更换词条再说一次
    EVENT_PLAY_LEARN_ABNORMAL_KV,                       // 学习异常播报 --> 学习异常，将重置学习
    EVENT_PLAY_DELETE_WAKEWORD_RECORD_KV,               // 清除唤醒词播报 --> 请清除学习唤醒词后，再开始学习
    EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_KV,       // 清除指令词播报 --> 请清除学习指令词后，再开始学习
    EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_KV,         // 清除免唤醒词播报 --> 请清除学习免唤醒词后，再开始学习
    EVENT_PLAY_DELETE_WAKEWORD_RECORD_FAIL_KV,          // 清除唤醒词失败播报 --> 清除唤醒词失败，将重置学习
    EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_FAIL_KV,  // 清除指令词失败播报 --> 清除指令词失败，将重置学习
    EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_FAIL_KV,    // 清除免唤醒词失败播报 --> 清除免唤醒词失败，将重置学习
    EVENT_PLAY_NO_LEARN_DATA_KV,                        // 无学习数据播报 --> 还没有学习数据
    EVENT_PLAY_WRITE_FLASH_FAIL,                        // 写入flash失败 --> 学习数据过多，未写入flash
} LearnEvent;

typedef enum {
    FLAG_START_SINGLE_LEARNING = 0,                 // 1 --> 开始单次学习
    FLAG_INSTRUCTION_WORD_LEARN_START,              // 1 --> 开始连续学习指令词
    FLAG_SINGLE_LEARNING_RECORD,                    // 1 --> 单次学习的学习记录
    FLAG_INSTRUCTION_WORD_LEARN_RECORD,             // 1 --> 指令词的学习记录
    FLAG_LEARNING_IN_PROGRESS,                      // 1 --> 正在学习
    FLAG_EXIT_SELF_LEARN,                           // 1 --> 终止学习, 中止学习指令词
    FLAG_WORDS0_RECORD = 16,                        // 1 --> 开始学习指定词0
    FLAG_WORDS1_RECORD,                             // 1 --> 开始学习指定词1
    FLAG_WORDS2_RECORD,                             // 1 --> 开始学习指定词2
    FLAG_WORDS3_RECORD,                             // 1 --> 开始学习指定词3
    FLAG_WORDS4_RECORD,                             // 1 --> 开始学习指定词4
    FLAG_WORDS5_RECORD,                             // 1 --> 开始学习指定词5
    FLAG_WORDS6_RECORD,                             // 1 --> 开始学习指定词6
    FLAG_WORDS7_RECORD,                             // 1 --> 开始学习指定词7
    FLAG_WORDS8_RECORD,                             // 1 --> 开始学习指定词8
    FLAG_WORDS9_RECORD,                             // 1 --> 开始学习指定词9
} LearnFlag;

typedef struct {
    unsigned int  learn_kv;     // 当前要学习词的kv
    unsigned char major;        // 标明是主唤醒词还是指令词
} __attribute__((packed)) SELF_LEARNING_RELATE_LIST;

typedef struct {
    int ralate_list_num;
    SELF_LEARNING_RELATE_LIST *relate_list;
} SELF_LEARNING_RELATE_LIST_INFO;

typedef struct {
    unsigned short labels[20];
    int label_len;
    int threshold;
    int major;
    int kws_value;
} __attribute__((aligned(16))) SELF_LEARNING_KWS;

typedef struct {
    unsigned int count;
    SELF_LEARNING_KWS *self_learning_kws;
} __attribute__((aligned(16))) SELF_LEARNING_KWS_LIST;

int LvpSelfLearnInit(void);
int LvpGetSelfLearnInfo(void);
int LvpSetSlefLearnStates(LearnFlag bit, int value);
int LvpGetSlefLearnStates(int bit);
int LvpLearnsIndividualWord(int learn_kws_value);
int LvpDeleteIndividualWord(int del_kws_value);
int LvpDeleteInstructionLearnRecord(void);
int LvpLearningStart(LVP_CONTEXT *context);
int LvpLearningIn(LVP_CONTEXT *context, float *s_ctc_decoder_window, int s_ctc_index);
int LvpLearningExit(LVP_CONTEXT *context);
int LvpLearningReset(void);
int LvpLearnResetDelay(void);
int LvpDoSelfLearnScore(LVP_CONTEXT *context, float *s_ctc_decoder_window, int s_ctc_index, int major);
int LvpGetSelfLearnInfoLength(void);

#endif // __SELF_LEARNING_H__