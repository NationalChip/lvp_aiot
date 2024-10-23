#ifndef __LVP_WRITE_CUSTOM_DATA_H__
#define __LVP_WRITE_CUSTOM_DATA_H__

#include "lvp_custom_space.h"

/**
 * @brief 记忆数据内容到flash
 *
 * @param data 要记忆到flash的数据内容
 * @param len 记忆的数据长度
 *
 * @retval 0 成功
 * @retval -1 失败
 */
int LvpWriteCustomData(unsigned char *data, int len);

/**
 * @brief 从flash读取记忆的数据长度，并返回保存在flash中的地址
 *
 * @param data_len 记忆的数据长度
 *
 * @retval unsigned char * 类型的指针，为记忆数据的访问地址
 */
unsigned char *LvpReadCustomData(int *data_len);

/**
 * @brief 获取数据在flash中的地址
 *
 * @param None
 *
 * @retval unsigned char * 类型的指针，为记忆数据的访问地址
 */
unsigned char *LvpGetCustomDataAddr(void);

#endif  /* __LVP_WRITE_CUSTOM_DATA_H__ */
