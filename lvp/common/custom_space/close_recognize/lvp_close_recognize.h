#ifndef __LVP_CLOSE_RECOGNIZE_H__
#define __LVP_CLOSE_RECOGNIZE_H__

#include <autoconf.h>

#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
typedef struct {
    int need_write_num;                                         // 不需要识别，需要写入记录的个数
    int need_write_kv[CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM];    // 不需要识别，需要写入的kv
} __attribute__((aligned(16))) CLOSE_KWS_NEED_WRITE_INFO;
#endif

typedef struct {
    int no_write_num;                                           // 不需要识别，不需要写入记录的个数
    int no_write_kv[CONFIG_NO_NEED_WRITE_FLASH_CLOSE_KV_NUM];   // 不需要识别，不需要写入的kv
} __attribute__((aligned(16))) CLOSE_KWS_NO_WRITE_INFO;

typedef struct {
#if CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM
    CLOSE_KWS_NEED_WRITE_INFO need_write_info;
#endif
    CLOSE_KWS_NO_WRITE_INFO   no_write_info;
} __attribute__((aligned(16))) CLOSE_KWS_INFO;

/**
 * @brief 初始化关闭指令词识别功能，从自定义空间位置读取相关信息
 *
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpInitCloseCmdRecognize(void);

/**
 * @brief 清空所有关闭的指令词信息
 *
 * @param None
 *
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpClearAllCloseInfo(void);

/**
 * @brief 通过数组指定开启或关闭的 kws_value
 * 注意：此接口会覆盖先前记录的所有值
 *
 * @param kws_value_array 要关闭指令词的kv值数组， 为 NULL 时将会情况所有记录的值
 * @param array_len 数组长度
 * @param write_flash_flag 0:不写入flash 1:写入flash，写入flash下次开机将也不能唤醒
 *
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpBatchProcesCmdRecognize(int *kws_value_array, int array_len, int write_flash_flag);

/**
 * @brief 关闭指定 kws_value 的识别。
 * 注意：当一个指令词 write_flash_flag 设为 1 已经关闭后，再使用设为 0 再次调用此接口会将上次的flash中此指令词清除
 *
 * @param kws_value 要关闭指令词的kv值
 * @param write_flash_flag 0:不写入flash 1:写入flash，写入flash下次开机将也不能唤醒
 *
 * @retval 1 当前关闭kv值已关闭不需要在关闭
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpCloseCmdRecognize(int kws_value, int write_flash_flag);

/**
 * @brief 打开指定 kws_value 的识别
 *
 * @param kws_value 要打开指令词的kv值
 *
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpOpenCmdRecognize(int kws_value);

/**
 * @brief 获取指定 kws_value 是否可以识别的状态
 *
 * @param kws_value 要获取的指令词的kv值
 *
 * @retval 1 不可识别，已关闭
 * @retval 0 可以识别，未关闭
 */
int LvpGetCloseStatus(int kws_value);

#endif /* __LVP_CLOSE_RECOGNIZE_H__ */