#include <autoconf.h>
#include <stdio.h>
#include <string.h>
#include <base_addr.h>
#include <lvp_attr.h>
#include <board_config.h>
#include <lvp_context.h>
#include <lvp_audio_in.h>

#include <driver/gx_flash.h>
#include <driver/gx_delay.h>
#include <driver/gx_timer.h>
#include <driver/gx_irq.h>
#include <driver/gx_snpu.h>

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
# include <lvp_tts_player.h>
#endif

#ifdef CONFIG_LVP_SELF_LEARN_UES_LED_TIP
# include <driver/gx_padmux.h>
# include <driver/gx_gpio.h>
# include <gpio_led/gpio_led.h>
#endif

#include "decoder.h"
#include "app_core/lvp_app_core.h"
#include "self_learning.h"

#ifndef CONFIG_LVP_HAS_RESOURCE_BIN
# error "resource_bin not set!!!"
#endif

#define LOG_TAG "[SELF_LEARNING]"

#define CONFIG_SELF_LEARN_MAX_FAIL_NUM  (3)         // 连续失败次数退出
#define CONFIG_NO_INSTRUCTION_WORD_NUM  (CONFIG_LVP_SELF_LEARN_WAKE_WORD_NUM + CONFIG_LVP_SELF_LEARN_IMMEDIATE_WORD_NUM)
#define CONFIG_LVP_SELF_LEARN_WORD_NUM  (CONFIG_NO_INSTRUCTION_WORD_NUM + CONFIG_LVP_SELF_LEARN_INSTRUCTION_WORD_NUM)
#define SELF_LEARN_IN_FLASH_LEN         (4 * 1024)
#define SELF_LEARN_START_FLASH_ADDR     (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH)
/*
 *  resource.bin 各部分信息存储位置
 *      自学习                     tts_list   uart_list    wav_data
 *  |4K zero + self_learn_info  | tts_info | uart_info | tts_wav_data |
 *
 *  开启关闭识别功能时 还会在之前多 CONFIG_CUSTOM_STORAGE_SPACE_LEN * 4K 零数据
 *  |4K zero 来存放可能要存放的数据 |4K zero + self_learn_info | tts_info | uart_info | tts_wav_data |
 */
#ifdef CONFIG_CUSTOM_STORAGE_SPACE
# define SELF_LEARN_INFO_FLASH_ADDR      (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH + CONFIG_CUSTOM_STORAGE_SPACE_LEN * 4096 + SELF_LEARN_IN_FLASH_LEN)
#else
# define SELF_LEARN_INFO_FLASH_ADDR      (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH + SELF_LEARN_IN_FLASH_LEN)
#endif
#define TO_XIP_ADDR(addr)               (unsigned char*)(CONFIG_FLASH_XIP_BASE + (unsigned int)addr)
#define TO_FLASH_ADDR(addr)             (unsigned char*)((unsigned int)addr - CONFIG_FLASH_XIP_BASE)

typedef struct {
    int self_learn_flag;
    int start_cnt_idx;          // 记录本次学习起始的 context下标，用来在时间很久未学习成功时退出
    int cur_learn_learn_kv;     // 记录本次学习的词的 kws_value
    int cur_learn_kws_major;    // 记录本次学习的词的 major
    int cur_learn_record_index; // 记录本次学习的词 在 self_learn_flag 中第几个bit记录已经学习
    int cur_learn_complete_num; // 记录本次学习过程中 学习完成次数
    int cur_learn_ok_cnt;       // 记录本次学习过程中 学习一个词期间学习成功的个数
    int cur_learn_error_cnt;    // 记录本次学习过程中 学习一个词期间学习失败的个数
    int cur_learn_label_cnt;    // 记录本次学习过程中 成功学习的 label 数
} LVP_SELF_LEARN;

static unsigned char *self_learn_info_addr = TO_XIP_ADDR(SELF_LEARN_INFO_FLASH_ADDR);   // 存放学习关系信息的地址
static unsigned char *self_learn_list_addr = TO_XIP_ADDR(SELF_LEARN_START_FLASH_ADDR);  // 存放学习到的词条信息地址
static APP_EVENT self_learn_event;
static LVP_SELF_LEARN s_self_learn_param;
static int s_delay[CONFIG_LVP_SELF_LEARN_WORD_NUM * CONFIG_LVP_SELF_LEARN_REPETITION_COUNT] = {0};
static SELF_LEARNING_KWS s_self_learn_kws[CONFIG_LVP_SELF_LEARN_WORD_NUM * CONFIG_LVP_SELF_LEARN_REPETITION_COUNT] = {0};
static SELF_LEARNING_KWS_LIST s_self_learn_list;
static SELF_LEARNING_RELATE_LIST_INFO s_relate_list_info;
static unsigned char self_learn_flag = 0;

int LvpSetSlefLearnStates(LearnFlag bit, int value)
{
    if (value == 1) {
        s_self_learn_param.self_learn_flag |= (1 << bit);
    } else if (value == 0) {
        s_self_learn_param.self_learn_flag &= ~(1 << bit);
    } else {
        return -1;
    }

    return 0;
}

int LvpGetSlefLearnStates(int bit)
{
    int result = 0;
    if (bit == FLAG_SINGLE_LEARNING_RECORD) {
        if ((s_self_learn_param.self_learn_flag >> FLAG_START_SINGLE_LEARNING) & 0x1) { // 在正在单次学习的时候，返回当前学习词是否学习
            result = ((s_self_learn_param.self_learn_flag >> s_self_learn_param.cur_learn_record_index) & 0x1);
        } else {     // 在未单次学习的时候，返回所有学习过得指令词记录
            result |=  ((s_self_learn_param.self_learn_flag >> FLAG_SINGLE_LEARNING_RECORD) & 0x1);
            for (int i = FLAG_WORDS0_RECORD; i < FLAG_WORDS9_RECORD; i++) {
                result |= ((s_self_learn_param.self_learn_flag >> i) & 0x1);
            }
        }
    } else if (bit == FLAG_INSTRUCTION_WORD_LEARN_RECORD) {
        result |= ((s_self_learn_param.self_learn_flag >> bit) & 0x1);
        for (int i = (FLAG_WORDS0_RECORD + CONFIG_NO_INSTRUCTION_WORD_NUM); i < FLAG_WORDS9_RECORD; i++) {
            result |= ((s_self_learn_param.self_learn_flag >> i) & 0x1);
        }
    } else {
        result = ((s_self_learn_param.self_learn_flag >> bit) & 0x1);
    }

    return result;
}

static int _SelfLearnEventHandler(APP_EVENT self_learn_event)
{
    printf(LOG_TAG"EVENT, %d\n", self_learn_event.event_id);

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerResponse(&self_learn_event);
#endif

#ifdef CONFIG_LVP_SELF_LEARN_UES_LED_TIP
    int flicker_count = 0;

    switch (self_learn_event.event_id)
    {
        case EVENT_START_LEARN_WORD0_KV:
        case EVENT_START_LEARN_WORD1_KV:
        case EVENT_START_LEARN_WORD2_KV:
        case EVENT_START_LEARN_WORD3_KV:
        case EVENT_START_LEARN_WORD4_KV:
        case EVENT_START_LEARN_WORD5_KV:
        case EVENT_START_LEARN_WORD6_KV:
        case EVENT_START_LEARN_WORD7_KV:
        case EVENT_START_LEARN_WORD8_KV:
        case EVENT_START_LEARN_WORD9_KV:                    // 开始学习指定词时，成功
            flicker_count = 2;
            break;
        case EVENT_DEL_WORD0_KV:
        case EVENT_DEL_WORD1_KV:
        case EVENT_DEL_WORD2_KV:
        case EVENT_DEL_WORD3_KV:
        case EVENT_DEL_WORD4_KV:
        case EVENT_DEL_WORD5_KV:
        case EVENT_DEL_WORD6_KV:
        case EVENT_DEL_WORD7_KV:
        case EVENT_DEL_WORD8_KV:
        case EVENT_DEL_WORD9_KV:                            // 删除指定词时，成功
            flicker_count = 2;
            break;
        case EVENT_START_LEARN_INSTRUCTION_WORD_KV:         // 开始学习指令词
            flicker_count = 2;
            break;
        case EVENT_CLEAR_LEARN_INSTRUCTION_WORD_KV:         // 删除所有指令词
            flicker_count = 2;
            break;
        case EVENT_RESET_LEARN_KV:                          // 重置学习时
            flicker_count = 2;
            break;
        case EVENT_EXIT_SELF_LEARN_KV:
        case EVENT_PLAY_LEARN_SUCCES_KV:
        case EVENT_PLAY_LEARN_COMPLETE_KV:                  // 学习时，成功
            flicker_count = 2;
            break;
        case EVENT_PLAY_LEARN_FAIL_KV:
        case EVENT_PLAY_LEARN_SAME_KV:                      // 学习时，失败
            flicker_count = 3;
            break;
        case EVENT_PLAY_LEARN_ABNORMAL_KV:
            flicker_count = 3;
            break;
        case EVENT_PLAY_DELETE_WAKEWORD_RECORD_KV:
        case EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_KV:
        case EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_KV:    // 开始学习，有记录的时
            flicker_count = 3;
            break;
        case EVENT_PLAY_NO_LEARN_DATA_KV:                   // 删除时，但没有学习数据时
            flicker_count = 2;
            break;
        case EVENT_PLAY_DELETE_WAKEWORD_RECORD_FAIL_KV:     // 删除记录失败时
        case EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_FAIL_KV:
        case EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_FAIL_KV:
            flicker_count = 3;
        break;

    default:
        break;
    }

    int ret = padmux_check(CONFIG_LVP_SELF_LEARN_LED_PIN_ID, CONFIG_LVP_SELF_LEARN_LED_PIN_ID == 2 ? 0 : 1);
    if (0 != ret) {
        printf("The led pins(%d) used have other multiplexes\n", CONFIG_LVP_SELF_LEARN_LED_PIN_ID);
    }
    padmux_set(CONFIG_LVP_SELF_LEARN_LED_PIN_ID, CONFIG_LVP_SELF_LEARN_LED_PIN_ID == 2 ? 0 : 1);
    gx_gpio_set_direction(CONFIG_LVP_SELF_LEARN_LED_PIN_ID, GX_GPIO_DIRECTION_OUTPUT);
    GpioLedFlicker(CONFIG_LVP_SELF_LEARN_LED_PIN_ID, 200, 200, flicker_count);
#endif

    return 0;
}

int LvpLearnsIndividualWord(int learn_kws_value)
{
    int relate_index = learn_kws_value - EVENT_START_LEARN_WORD0_KV;

    LvpSetSlefLearnStates(FLAG_START_SINGLE_LEARNING, 1);
    s_self_learn_param.cur_learn_learn_kv     = s_relate_list_info.relate_list[relate_index].learn_kv;
    s_self_learn_param.cur_learn_record_index = FLAG_WORDS0_RECORD + relate_index;
    s_self_learn_param.cur_learn_kws_major    = s_relate_list_info.relate_list[relate_index].major;

    printf(LOG_TAG"Learn Individual Word, kv: %d, record_index: %d, major: %d\n",
                                s_self_learn_param.cur_learn_learn_kv,
                                s_self_learn_param.cur_learn_record_index,
                                s_self_learn_param.cur_learn_kws_major);

    return 0;
}

int LvpLearnResetDelay(void)
{
    memset(s_delay, 0, sizeof(s_delay));

    return 0;
}

static float _LvpGetSelfLearnCtcScore(float *s_ctc_decoder_window, int s_ctc_index, unsigned short *labels, int label_length)
{
    int valid_frame_num = CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
    int idx = s_ctc_index % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
    int start_index = 0;

    float ctc_score = LvpFastCtcBlockScorePlus(s_ctc_decoder_window + (idx * CONFIG_KWS_MODEL_OUTPUT_LENGTH)
            , valid_frame_num - idx
            , s_ctc_decoder_window
            , idx
            , CONFIG_KWS_MODEL_OUTPUT_LENGTH
            , CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1
            , labels
            , label_length
            , &start_index);

    return ctc_score;
}

int LvpDoSelfLearnScore(LVP_CONTEXT *context, float *s_ctc_decoder_window, int s_ctc_index, int major)
{
    for (int i = 0; i < s_self_learn_list.count; i++) {
        int cur_word_major = s_self_learn_list.self_learning_kws[i].major;
        if (((cur_word_major & 0xf) == 2) || ((cur_word_major & 0xf) == major) || major == 0) {
            float ctc_score = _LvpGetSelfLearnCtcScore(s_ctc_decoder_window
                                                    , s_ctc_index
                                                    , s_self_learn_list.self_learning_kws[i].labels
                                                    , s_self_learn_list.self_learning_kws[i].label_len);

            if (((ctc_score * 10) >= s_self_learn_list.self_learning_kws[i].threshold) && (ctc_score < 100.f)) {
                int delay_num = (cur_word_major & 0xf0000) >> 16;
                if ((s_delay[i] < delay_num)) {
                    s_delay[i]++;
                    printf("delay_num: %d\n", delay_num);
                    printf (LOG_TAG"ctx:%d,delay:%d\n", context->ctx_index, s_delay[i]);
                    continue;
                }

                context->kws = s_self_learn_list.self_learning_kws[i].kws_value;
                ResetCtcWinodw();
                LvpLearnResetDelay();
                printf (LOG_TAG"index[%d] Activation ctx:%d,Kws:[%d],th:%d,S:%d\n", i
                        , context->ctx_index
                        , s_self_learn_list.self_learning_kws[i].kws_value
                        , s_self_learn_list.self_learning_kws[i].threshold
                        , (int)(10*ctc_score));
            }
        }
    }

    return 0;
}

static int _Argmax(float* array, int length)
{
    int max_index = 0;
    float max_value = array[0];
    for (int i = 1; i < length; ++i) {
        if (array[i] > max_value) {
            max_value = array[i];
            max_index = i;
        }
    }

    return max_index;
}

static int _TrimArrayWithoutOnes(unsigned short *array, int length) {
    int start = -1; // 初始化为-1，表示还未找到第一个1
    int end = -1; // 初始化为-1，表示还未找到最后一个1
    int i;

    // 找到第一个1之后的位置
    for (i = 0; i < length; i++) {
        if (array[i] == 1) {
            start = i + 1;
            break;
        }
    }

    // 找到最后一个1
    if (start == length) {
        return length - 1;
    }

    // 如果没有找到第一个1，返回0
    if (start == -1) {
        return length;
    }

    // 从后往前找到第一个1之前的位置
    for (i = length - 1; i >= start; i--) {
        if (array[i] == 1) {
            end = i;
            break;
        }
    }

    // 如果没有找到最后一个1，直到末尾
    if (end == -1) {
        end = length;
    }

    // 移动数组元素，删除两个1之间的元素
    int newLength = end - start;
    for (i = 0; i < newLength; i++) {
        array[i] = array[start + i];
    }

    return newLength;
}

static unsigned short decoded_sequence[CONFIG_KWS_MODEL_DECODER_WIN_LENGTH] = {0};
static void _LvpCtcGreedyDecoder(float* probs, int s_ctc_index, int blank_label, int* decoded_length) {
    int num_classes = CONFIG_KWS_MODEL_OUTPUT_LENGTH;
    int decoded_index = 0;
    int prev_label = -1;
    int ctc_window_end_idx = CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - 1;
    int start_index = (s_ctc_index != ctc_window_end_idx) ? s_ctc_index + 1 : 0;
    int end_index = (s_ctc_index != ctc_window_end_idx) ? CONFIG_KWS_MODEL_DECODER_WIN_LENGTH : s_ctc_index;
    int best_label = 0;

    memset(decoded_sequence, 0, sizeof(decoded_sequence));

    for (int t = start_index; t < end_index; ++t) {
        best_label = _Argmax(probs + t * num_classes, num_classes);

        if (best_label != blank_label && best_label != prev_label) {
            decoded_sequence[decoded_index++] = best_label;
        }

        prev_label = best_label;
    }
    for (int t = 0; t < start_index - 1; ++t) {
        best_label = _Argmax(probs + t * num_classes, num_classes);

        if (best_label != blank_label && best_label != prev_label) {
            decoded_sequence[decoded_index++] = best_label;
        }

        prev_label = best_label;
    }
    *decoded_length = _TrimArrayWithoutOnes(decoded_sequence, decoded_index);
}

static int _LvpWriteListToFlash(void)
{
    // 防止写入flash的数据超过预留空间
    if (sizeof(int) + s_self_learn_list.count * sizeof(SELF_LEARNING_KWS) > SELF_LEARN_IN_FLASH_LEN) {
        printf("Too much learned data failed to save\n");
        return -2;
    }

    int ret = 0;
    uint32_t irq_state = gx_lock_irq_save();

    gx_snpu_pause();

    printf(LOG_TAG"Addr: %#x, %#x, %#x\n", s_self_learn_kws, &s_self_learn_list.self_learning_kws[0], s_self_learn_list.self_learning_kws);
    printf(LOG_TAG"Write Self Learn Kws List [total: %d]\n", s_self_learn_list.count);

    GX_FLASH_DEV *flash = gx_spi_flash_probe(CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    unsigned int flash_size = gx_spi_flash_getinfo(flash, GX_FLASH_CHIP_SIZE);

    unsigned char otp[5] = {0};
    if (gx_spi_flash_otp_read(flash, 0, otp, 5) < 0) {
        printf("read flash otp error\n");
    }

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) { // 8003需要解写保护
        gx_spi_flash_write_protect_unlock(flash);
    }

    ret = gx_spinor_flash_erasedata(SELF_LEARN_START_FLASH_ADDR, SELF_LEARN_IN_FLASH_LEN);
    if (ret == -1) return -1;

    ret = gx_spinor_flash_pageprogram(SELF_LEARN_START_FLASH_ADDR
                                    , (unsigned char*)&s_self_learn_list.count
                                    , sizeof(int));
    if (ret == -1) return -1;

    ret = gx_spinor_flash_pageprogram((SELF_LEARN_START_FLASH_ADDR + sizeof(int))
                                    , (unsigned char*)s_self_learn_list.self_learning_kws
                                    , s_self_learn_list.count * sizeof(SELF_LEARNING_KWS));
    if (ret == -1) return -1;
    SELF_LEARNING_KWS_LIST s_read_list;
    s_read_list.count = *(int*)self_learn_list_addr;
    s_read_list.self_learning_kws = (SELF_LEARNING_KWS*)(self_learn_list_addr + sizeof(int));

    printf(LOG_TAG"Read Self Learn Kws List [total: %d]\n", s_read_list.count);
    for (int i = 0; i < s_read_list.count; ++i) {
        printf("RKV: %02d | ", s_read_list.self_learning_kws[i].kws_value);
        printf("RTH: %02d | ", s_read_list.self_learning_kws[i].threshold);
        printf("RMJ: %02d | L: [", s_read_list.self_learning_kws[i].major);
        for (int j = 0; j < s_read_list.self_learning_kws[i].label_len; j++) {
            printf("%hd ", s_read_list.self_learning_kws[i].labels[j]);
        }
        printf("]\n");
    }

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) {
        unsigned int actual_protect = 0;
        gx_spi_flash_write_protect_status(flash, &actual_protect);
        if (actual_protect != (flash_size + 4*1024)) {
            unsigned long protect = flash_size + 4*1024;
            gx_spi_flash_write_protect_lock(flash, protect);
        }
    }

    gx_snpu_resume();
    gx_unlock_irq_restore(irq_state);

    return ret;
}

static int _LvpCmpLabel(const unsigned short *s1, int l1, const unsigned short *s2, int l2)
{
    int i = 0, j = 0;
    int same_cnt = 0;
    int last_index = 0;

    for (i = 0; i < l2; i++) {
        for (j = last_index; j < l1; j++) {
            if (s2[i] == s1[j] ) {
                same_cnt ++;
                last_index = j + 1;
                break;
            }
        }
    }

    return same_cnt;
}

static int _LvpGetLabel(float *s_ctc_decoder_window, int s_ctc_index, int major, int kws_value)
{
    int i = 0;
    int decoded_length = 0;
    int ctc_socre = 0;
    int blank_label = CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1;

    _LvpCtcGreedyDecoder(s_ctc_decoder_window, s_ctc_index, blank_label, &decoded_length);
    ctc_socre = (int)(_LvpGetSelfLearnCtcScore(s_ctc_decoder_window
                                                , s_ctc_index
                                                , decoded_sequence
                                                , decoded_length) * 10);

    if (decoded_length < 4) {
        return EVENT_LEARN_CONTINUE_KV;
    }

    printf(LOG_TAG"major: %d, kws_value: %d, learn num: %d, socre: %d\n", major, kws_value, s_self_learn_list.count, ctc_socre);
    printf(LOG_TAG"get label len: %d, data: \n", decoded_length);
    for (int i = 0; i < decoded_length; ++i) {
        printf("%hd, ", decoded_sequence[i]);
    }
    printf("\n");

    if ((decoded_length > 18) || (ctc_socre < 600)) {
        printf(LOG_TAG"Learning failure\n");
        s_self_learn_param.cur_learn_error_cnt++;

        return EVENT_PLAY_LEARN_FAIL_KV;
    }

    for (i = 0; i < s_self_learn_list.count; i++) {
        int label_same_num = _LvpCmpLabel(s_self_learn_kws[i].labels, s_self_learn_kws[i].label_len, decoded_sequence, decoded_length);
        int cur_learn_flag = s_self_learn_kws[i].kws_value == kws_value ? 1 : 0;
        int least_identical_num = decoded_length / 2; /* 当相同的标签个数大于 本次标签个数的 1/2 认为接近 */
        printf(LOG_TAG"i: %d, cur_learn_flag： %d, label_same_num: %d, decode_len: %d\n", i, cur_learn_flag, label_same_num, decoded_length);

        if (label_same_num == decoded_length && !cur_learn_flag) { // 本次学习与之前学习的指令词一致，认为失败
            printf(LOG_TAG"Learning label Abnormal\n");
            s_self_learn_param.cur_learn_error_cnt++;
            return EVENT_PLAY_LEARN_SAME_KV;
        } else if (cur_learn_flag && label_same_num < least_identical_num && (i == s_self_learn_list.count - 1)) { // 本次学习与之前学习的结果相差较大，认为失败
            printf(LOG_TAG"Learning label diff\n");
            s_self_learn_param.cur_learn_error_cnt++;
            return EVENT_PLAY_LEARN_FAIL_KV;
        } else if (cur_learn_flag && label_same_num == decoded_length) { // 本次学习与之前学习的指令词一致，不需要再写一个标签
            s_self_learn_param.cur_learn_ok_cnt++;
            s_self_learn_param.cur_learn_error_cnt = 0; // 本次学习成功,将学习失败次数置0,达到连续3次失败的时候异常退出
            printf(LOG_TAG"Learning label same\n");
            return EVENT_PLAY_LEARN_SUCCES_KV;
        } else if (cur_learn_flag && label_same_num >= least_identical_num) { // 多次学习，其中有一次得到的标签相同个数多于4认为相近
            printf(LOG_TAG"Learning label xiangjian\n");
            break;
        }
    }
    if (i <= s_self_learn_list.count) {
        printf(LOG_TAG"Get new label, cur_count %d\n", s_self_learn_list.count);
        memcpy(s_self_learn_kws[s_self_learn_list.count].labels, decoded_sequence, sizeof(short) * decoded_length);
        s_self_learn_kws[s_self_learn_list.count].label_len = decoded_length;
        if (decoded_length < 7)
            major |= 3<<16;
        printf(LOG_TAG"delay major %#x\n", major);
        s_self_learn_kws[s_self_learn_list.count].major = major;
        s_self_learn_kws[s_self_learn_list.count].kws_value = kws_value;

        if (ctc_socre < 750) {
            ctc_socre = 750;
        }

        if ((major & 0xf) == 2 && decoded_length < 7) { // 免唤醒词并且字数少于等于3个
            printf(LOG_TAG"set immediate score\n");
            s_self_learn_kws[s_self_learn_list.count].threshold = ctc_socre;
        } else {
            printf(LOG_TAG"set command score\n");
            if (ctc_socre > 960) {
                s_self_learn_kws[s_self_learn_list.count].threshold = ctc_socre - 45;
            } else if (ctc_socre > 940) {
                s_self_learn_kws[s_self_learn_list.count].threshold = ctc_socre - 30;
            } else if (ctc_socre > 900) {
                s_self_learn_kws[s_self_learn_list.count].threshold = ctc_socre - 20;
            } else {
                s_self_learn_kws[s_self_learn_list.count].threshold = ctc_socre;
            }
        }

        s_self_learn_list.count++;
        s_self_learn_param.cur_learn_label_cnt ++;
    }

    s_self_learn_param.cur_learn_ok_cnt++;
    s_self_learn_param.cur_learn_error_cnt = 0; // 本次学习成功,将学习失败次数置0,达到连续3次失败的时候异常退出

    printf(LOG_TAG"Learning success\n");

    return EVENT_PLAY_LEARN_SUCCES_KV;
}

// 学习开始
int LvpLearningStart(LVP_CONTEXT *context)
{
    self_learn_event.ctx_index = context->ctx_index;

    // 有学习信息时提示清除信息后再进行学习
    if (LvpGetSlefLearnStates(FLAG_START_SINGLE_LEARNING) &&
        LvpGetSlefLearnStates(s_self_learn_param.cur_learn_record_index)) {
        printf(LOG_TAG"Please clear record!\n");
        int cur_relate_index = s_self_learn_param.cur_learn_record_index - FLAG_WORDS0_RECORD;
        if (s_relate_list_info.relate_list[cur_relate_index].major == 1)
            self_learn_event.event_id = EVENT_PLAY_DELETE_WAKEWORD_RECORD_KV;
        else if (s_relate_list_info.relate_list[cur_relate_index].major == 0)
            self_learn_event.event_id = EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_KV;
        else
            self_learn_event.event_id = EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_KV;
        LvpSetSlefLearnStates(FLAG_START_SINGLE_LEARNING, 0);           // 若学习过,学习唤醒词|指令词标志位清0,退出
    } else if (LvpGetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_START) &&
               LvpGetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_RECORD)) {
        printf(LOG_TAG"Please clear instruction record!\n");
        self_learn_event.event_id = EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_KV;
        LvpSetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_START, 0);    // 若学习过,学习指令词标志位清0,退出
    } else {
        LvpSetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS, 1);
        ResetCtcWinodw();
        if (LvpGetSlefLearnStates(FLAG_START_SINGLE_LEARNING)) {
            self_learn_event.event_id = EVENT_START_LEARN_WORD0_KV + s_self_learn_param.cur_learn_record_index - FLAG_WORDS0_RECORD;
        } else {
            self_learn_event.event_id = EVENT_START_LEARN_WORD0_KV + CONFIG_NO_INSTRUCTION_WORD_NUM;
        }
    }
    _SelfLearnEventHandler(self_learn_event);

    return 0;
}

static int silence_flag = 0;
static int _CheckSilence(float *probs, int idx)
{
    float *prob = probs + idx * CONFIG_KWS_MODEL_OUTPUT_LENGTH;

    if (prob[CONFIG_KWS_MODEL_OUTPUT_LENGTH - 1] > 0.95) {
        silence_flag ++;
    } else {
        silence_flag = 0;
    }

    return silence_flag >= CONFIG_LVP_MUTE_NUM ? 1 : 0; // 连续 CONFIG_LVP_MUTE_NUM * context 次静音认为静音
}

int LvpLearningIn(LVP_CONTEXT *context, float *s_ctc_decoder_window, int s_ctc_index)
{
    if (s_self_learn_param.start_cnt_idx == 0) {
        s_self_learn_param.start_cnt_idx = context->ctx_index;
        printf(LOG_TAG"learn start context index %d, list_count: %d\n", s_self_learn_param.start_cnt_idx, s_self_learn_list.count);
    }

    int ctc_index     = (s_ctc_index - 1) % CONFIG_KWS_MODEL_DECODER_WIN_LENGTH;
    int cur_learn_cnt = context->ctx_index - s_self_learn_param.start_cnt_idx;

    // 在填满一个窗之前不进行判断静音退出
    if (cur_learn_cnt < CONFIG_KWS_MODEL_DECODER_WIN_LENGTH - 4) {
        return 0;
    }

    // 在填满一个窗后进行判断静音，超时需要退出
    if (_CheckSilence(s_ctc_decoder_window, ctc_index) == 0 && cur_learn_cnt < 100) {
        return 0;
    }
    silence_flag = 0;
    printf(LOG_TAG"silence\n");

    int major = 0;
    int kws_value = 0;
    int set_start_flag = 0;
    int set_record_flag = 0;

    if (LvpGetSlefLearnStates(FLAG_START_SINGLE_LEARNING)) {    // 单次学习
        major           = s_self_learn_param.cur_learn_kws_major;
        kws_value       = s_self_learn_param.cur_learn_learn_kv;
        set_start_flag  = FLAG_START_SINGLE_LEARNING;
        set_record_flag = s_self_learn_param.cur_learn_record_index;
    } else {                                                    // 连续学习
        int learn_index = s_self_learn_param.cur_learn_complete_num + CONFIG_NO_INSTRUCTION_WORD_NUM;
        major           = 0;
        kws_value       = s_relate_list_info.relate_list[learn_index].learn_kv;
        set_start_flag  = FLAG_INSTRUCTION_WORD_LEARN_START;
        set_record_flag = FLAG_WORDS0_RECORD + CONFIG_NO_INSTRUCTION_WORD_NUM;
    }

    self_learn_event.ctx_index = context->ctx_index;
    // self_learn_event.event_id = _LvpGetLabel(s_ctc_decoder_window, ctc_index, major, kws_value + s_self_learn_param.cur_learn_complete_num);
    self_learn_event.event_id = _LvpGetLabel(s_ctc_decoder_window, ctc_index, major, kws_value);
    if (self_learn_event.event_id == EVENT_LEARN_CONTINUE_KV) {
        if (cur_learn_cnt < 100) {
            return 0;
        } else {
            printf(LOG_TAG"Failed to learn within 6 seconds\n");
            self_learn_event.event_id = EVENT_PLAY_LEARN_FAIL_KV;
            s_self_learn_param.cur_learn_error_cnt++;
        }
    }

    printf(LOG_TAG"# info [ok: %d], [error: %d], [learn_time: %d], [complete_num: %d], [result: %d]\n",
                    s_self_learn_param.cur_learn_ok_cnt, s_self_learn_param.cur_learn_error_cnt,
                    cur_learn_cnt, s_self_learn_param.cur_learn_complete_num, self_learn_event.event_id);

    s_self_learn_param.start_cnt_idx = 0;
    ResetCtcWinodw();

    if (s_self_learn_param.cur_learn_ok_cnt >= CONFIG_LVP_SELF_LEARN_REPETITION_COUNT) {
        s_self_learn_param.cur_learn_complete_num++;
        s_self_learn_param.cur_learn_error_cnt = 0;
        s_self_learn_param.cur_learn_ok_cnt = 0;

        if ((set_start_flag == FLAG_START_SINGLE_LEARNING) ||           // 单次学习时
            (s_self_learn_param.cur_learn_complete_num >= (CONFIG_LVP_SELF_LEARN_INSTRUCTION_WORD_NUM))) {  // 连续学习指令词时
            self_learn_event.event_id = EVENT_PLAY_LEARN_COMPLETE_KV;
            s_self_learn_list.self_learning_kws = s_self_learn_kws;
            s_self_learn_param.cur_learn_label_cnt = 0;
            s_self_learn_param.cur_learn_complete_num = 0;
            s_self_learn_param.cur_learn_learn_kv = 0;
            s_self_learn_param.cur_learn_record_index = 0;
            s_self_learn_param.cur_learn_kws_major = 0;
            LvpSetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS, 0);
            LvpSetSlefLearnStates(set_start_flag, 0);
            int ret = _LvpWriteListToFlash();
            if (ret == -1) {        // 写入时失败
                self_learn_event.event_id = EVENT_PLAY_LEARN_ABNORMAL_KV;
                _SelfLearnEventHandler(self_learn_event);
                printf(LOG_TAG"_LvpWriteListToFlash error!\n");
                LvpLearningReset();
                return -1;
            } else if (ret == -2) { // 写入超过预留空间未写入
                self_learn_event.event_id = EVENT_PLAY_WRITE_FLASH_FAIL;
                _SelfLearnEventHandler(self_learn_event);
                return -1;
            }

            LvpSetSlefLearnStates(set_record_flag, 1);
        } else {
            // 播放下一个学习的指令词播报
            self_learn_event.event_id = EVENT_START_LEARN_WORD0_KV + CONFIG_NO_INSTRUCTION_WORD_NUM + s_self_learn_param.cur_learn_complete_num;
        }
    }

    if (s_self_learn_param.cur_learn_error_cnt >= CONFIG_SELF_LEARN_MAX_FAIL_NUM) {
        s_self_learn_list.count -= s_self_learn_param.cur_learn_label_cnt;
        printf(LOG_TAG"## Learning failure exit. complete_num: %d, list_count: %d\n", s_self_learn_param.cur_learn_complete_num, s_self_learn_list.count);
        self_learn_event.event_id = EVENT_EXIT_SELF_LEARN_KV;
        s_self_learn_param.cur_learn_ok_cnt = 0;
        s_self_learn_param.cur_learn_error_cnt = 0;
        s_self_learn_param.cur_learn_complete_num = 0;
        s_self_learn_param.cur_learn_label_cnt = 0;
        LvpSetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS, 0);
        LvpSetSlefLearnStates(set_start_flag, 0);
    }

    _SelfLearnEventHandler(self_learn_event);

    return 0;
}

// 退出学习
int LvpLearningExit(LVP_CONTEXT *context)
{
    printf(LOG_TAG"Exit learning\n");

    int single_learn = LvpGetSlefLearnStates(FLAG_START_SINGLE_LEARNING) == 1 ? 1 : 0;
    int set_start_flag  = (single_learn == 1 ? FLAG_START_SINGLE_LEARNING  : FLAG_INSTRUCTION_WORD_LEARN_START);
    int set_record_flag = (single_learn == 1 ? FLAG_SINGLE_LEARNING_RECORD : FLAG_INSTRUCTION_WORD_LEARN_RECORD);

    printf(LOG_TAG"Exit info, single_learn, %d, set_start_flag %d, set_record_flag %d\n", single_learn, set_start_flag, set_record_flag);

    self_learn_event.event_id = EVENT_EXIT_SELF_LEARN_KV;
    s_self_learn_list.self_learning_kws = s_self_learn_kws;
    s_self_learn_param.cur_learn_complete_num = 0;
    s_self_learn_param.cur_learn_error_cnt = 0;
    s_self_learn_param.cur_learn_ok_cnt = 0;
    s_self_learn_param.start_cnt_idx = 0;
    s_self_learn_param.cur_learn_label_cnt = 0;
    s_self_learn_param.cur_learn_learn_kv = 0;
    s_self_learn_param.cur_learn_record_index = 0;
    s_self_learn_param.cur_learn_kws_major = 0;

    if (s_self_learn_list.count) {
        int ret = _LvpWriteListToFlash();
        if (ret == -1) {        // 写入时失败
            self_learn_event.event_id = EVENT_PLAY_LEARN_ABNORMAL_KV;
            _SelfLearnEventHandler(self_learn_event);
            printf(LOG_TAG"_LvpWriteListToFlash error!\n");
            LvpLearningReset();
            return -1;
        } else if (ret == -2) { // 写入超过预留空间未写入
            self_learn_event.event_id = EVENT_PLAY_WRITE_FLASH_FAIL;
            _SelfLearnEventHandler(self_learn_event);
            return -1;
        }
        LvpSetSlefLearnStates(set_record_flag, 1);
    }

    LvpSetSlefLearnStates(FLAG_LEARNING_IN_PROGRESS, 0);
    LvpSetSlefLearnStates(set_start_flag, 0);
    _SelfLearnEventHandler(self_learn_event);

    return 0;
}

// 重置学习
int LvpLearningReset(void)
{
    printf(LOG_TAG"Reset learning record!\n");

    int ret = 0;
    s_self_learn_list.count = 0;
    memset(s_self_learn_kws, 0, sizeof(s_self_learn_kws));
    memset(&s_self_learn_param, 0, sizeof(s_self_learn_param));
    ResetCtcWinodw();

    uint32_t irq_state = gx_lock_irq_save();
    gx_snpu_pause();
    gx_spinor_flash_init();
    GX_FLASH_DEV *flash   = gx_spi_flash_probe(CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    unsigned int flash_size = gx_spi_flash_getinfo(flash, GX_FLASH_CHIP_SIZE);

    unsigned char otp[5] = {0};
    if (gx_spi_flash_otp_read(flash, 0, otp, 5) < 0) {
        printf("read flash otp error\n");
    }

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) { // 8003需要解写保护
        gx_spi_flash_write_protect_unlock(flash);
    }

    ret = gx_spinor_flash_erasedata(SELF_LEARN_START_FLASH_ADDR, SELF_LEARN_IN_FLASH_LEN);
    if (ret == -1) return -1;

    ret = gx_spinor_flash_pageprogram(SELF_LEARN_START_FLASH_ADDR
                                    , (unsigned char*)&s_self_learn_list.count
                                    , sizeof(int));
    if (ret == -1) return -1;

    if (strncmp((const char*)otp, "8003A", strlen("8003A")) == 0) {
        unsigned int actual_protect = 0;
        gx_spi_flash_write_protect_status(flash, &actual_protect);
        if (actual_protect != (flash_size + 4*1024)) {
            unsigned long protect = flash_size + 4*1024;
            gx_spi_flash_write_protect_lock(flash, protect);
        }
    }

    gx_snpu_resume();
    gx_unlock_irq_restore(irq_state);

    self_learn_event.event_id = EVENT_RESET_LEARN_KV;
    _SelfLearnEventHandler(self_learn_event);


    return ret;
}

int LvpGetSelfLearnInfo(void)
{
    s_self_learn_list.count = *(int*)self_learn_list_addr;

    printf(LOG_TAG"Read Self Learn Kws List [total: %d]\n", s_self_learn_list.count);

    if (s_self_learn_list.count <= 0) {
        return 0;
    }

    memcpy(s_self_learn_kws, (SELF_LEARNING_KWS*)(self_learn_list_addr + sizeof(int)), s_self_learn_list.count * sizeof(SELF_LEARNING_KWS));
    s_self_learn_list.self_learning_kws = s_self_learn_kws;

    for (int i = 0; i < s_self_learn_list.count; i++) {
        for (int j = 0; j < CONFIG_LVP_SELF_LEARN_WORD_NUM; j++) {
            if (s_self_learn_kws[i].kws_value == s_relate_list_info.relate_list[j].learn_kv) {
                LvpSetSlefLearnStates(FLAG_WORDS0_RECORD+j, 1);
            }
        }
        printf(LOG_TAG"RKV: %03d | ", s_self_learn_kws[i].kws_value);
        printf("RTH: %03d | ", s_self_learn_kws[i].threshold);
        printf("RMJ: %03d | L: [", s_self_learn_kws[i].major);

        for (int j = 0; j < s_self_learn_kws[i].label_len; j++) {
            printf("%hd ", s_self_learn_kws[i].labels[j]);
        }
        printf("]\n");
    }

    return 0;
}

int LvpDeleteIndividualWord(int del_kws_value)
{
    printf(LOG_TAG"LvpDeleteIndividualWord\n");

    int relate_index     = del_kws_value - EVENT_DEL_WORD0_KV;
    int cur_del_kv       = s_relate_list_info.relate_list[relate_index].learn_kv;
    int cur_del_major    = s_relate_list_info.relate_list[relate_index].major & 0xf;
    int play_event_id    = del_kws_value;
    int cur_record_index = FLAG_WORDS0_RECORD + relate_index;
    int cur_word_cnt     = 0, start_index = 0, end_index = 0, move_num = 0;

    for (int i = 0; i < s_self_learn_list.count; i++) {
        if (s_self_learn_list.self_learning_kws[i].kws_value == cur_del_kv) {
            cur_word_cnt++;
            if (cur_word_cnt == 1) {
                start_index = i;
                end_index = i;
            } else {
                end_index = i;
            }
        }
    }

    if (0 == cur_word_cnt) {
        self_learn_event.event_id = EVENT_PLAY_NO_LEARN_DATA_KV;
        _SelfLearnEventHandler(self_learn_event);

        return 0;
    }

    move_num = s_self_learn_list.count - end_index - 1;
    printf(LOG_TAG"relate_index:%d, cur_word_cnt:%d, s_i:%d, e_i:%d, move_num:%d\n", relate_index, cur_word_cnt, start_index, end_index, move_num);
    printf(LOG_TAG"cur_del_kv:%d, cur_del_major:%d, play_event_id:%d, cur_record_index:%d\n", cur_del_kv, cur_del_major, play_event_id, cur_record_index);
    memmove(&s_self_learn_list.self_learning_kws[start_index], &s_self_learn_list.self_learning_kws[end_index+1], move_num * sizeof(SELF_LEARNING_KWS));
    s_self_learn_list.count -= cur_word_cnt;

    int ret = _LvpWriteListToFlash();
    if (ret == -1) {
        switch (cur_del_major)
        {
        case 3:
            self_learn_event.event_id = EVENT_PLAY_DELETE_IMMEDIATE_RECORD_WORD_FAIL_KV;
            break;
        case 1:
            self_learn_event.event_id = EVENT_PLAY_DELETE_WAKEWORD_RECORD_FAIL_KV;
            break;
        case 0:
            self_learn_event.event_id = EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_FAIL_KV;
            break;
        default:
            break;
        }
        printf(LOG_TAG"_LvpWriteListToFlash error!\n");
        _SelfLearnEventHandler(self_learn_event);
        return -1;
    } else if (ret == -2) { // 写入超过预留空间未写入
        self_learn_event.event_id = EVENT_PLAY_WRITE_FLASH_FAIL;
        _SelfLearnEventHandler(self_learn_event);
        return -1;
    }
    LvpSetSlefLearnStates(cur_record_index, 0);
    LvpGetSelfLearnInfo();
    self_learn_event.event_id = play_event_id;
    _SelfLearnEventHandler(self_learn_event);

    return 0;
}

int LvpDeleteInstructionLearnRecord(void)
{
    printf(LOG_TAG"LvpDeleteInstructionLearnRecord\n");

    int major_cnt = 0, start_index = 0, end_index = 0;

    for (int i = 0; i < s_self_learn_list.count; i++) {
        if ((s_self_learn_list.self_learning_kws[i].major & 0xf) == 1) {
            major_cnt++;
            if (major_cnt == 1) {
                start_index = i;
                end_index = i;
            } else {
                end_index = i;
            }
        }
    }

    if (s_self_learn_list.count == major_cnt) {
        self_learn_event.event_id = EVENT_PLAY_NO_LEARN_DATA_KV;
        _SelfLearnEventHandler(self_learn_event);

        return 0;
    }

    printf(LOG_TAG"major_cnt: %d, s_i: %d, e_i: %d\n", major_cnt, start_index, end_index);
    s_self_learn_list.self_learning_kws = &s_self_learn_kws[start_index];
    s_self_learn_list.count = major_cnt;

    int ret = _LvpWriteListToFlash();
    if (ret == -1) {        // 写入时失败
        self_learn_event.event_id = EVENT_PLAY_DELETE_INSTRUCTION_RECORD_WORD_FAIL_KV;
        printf(LOG_TAG"_LvpWriteListToFlash error!\n");
        _SelfLearnEventHandler(self_learn_event);
        LvpLearningReset();
        return -1;
    } else if (ret == -2) { // 写入超过预留空间未写入
        self_learn_event.event_id = EVENT_PLAY_WRITE_FLASH_FAIL;
        _SelfLearnEventHandler(self_learn_event);
        return -1;
    }
    LvpSetSlefLearnStates(FLAG_INSTRUCTION_WORD_LEARN_RECORD, 0);
    for (int i = FLAG_WORDS0_RECORD + CONFIG_NO_INSTRUCTION_WORD_NUM; i < FLAG_WORDS9_RECORD; i++) {
        LvpSetSlefLearnStates(i, 0);
    }

    LvpGetSelfLearnInfo();
    self_learn_event.event_id = EVENT_CLEAR_LEARN_INSTRUCTION_WORD_KV;
    _SelfLearnEventHandler(self_learn_event);

    return 0;
}

int LvpGetSelfLearnInfoLength(void)
{
    return sizeof(int) + s_relate_list_info.ralate_list_num * sizeof(SELF_LEARNING_RELATE_LIST) + SELF_LEARN_IN_FLASH_LEN;
}

int LvpSelfLearnInit(void)
{
    s_relate_list_info.ralate_list_num = *(int*)self_learn_info_addr;
    s_relate_list_info.relate_list = (SELF_LEARNING_RELATE_LIST*)(self_learn_info_addr + sizeof(int));

    if (self_learn_flag == 1) {
        return 0;
    }
    self_learn_flag = 1;

    printf(LOG_TAG"[self_learning_num]: %d\n", s_relate_list_info.ralate_list_num);
    for (int i = 0; i < s_relate_list_info.ralate_list_num; i++) {
        printf("[kws_value]%d, [major_type]%d\n", \
                s_relate_list_info.relate_list[i].learn_kv, s_relate_list_info.relate_list[i].major);
    }

    LvpGetSelfLearnInfo();

#ifdef CONFIG_LVP_HAS_TTS_PLAYER
    LvpTtsPlayerInit();
#endif
    LvpLearnResetDelay();

    return 0;
}
