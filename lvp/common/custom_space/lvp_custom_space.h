#ifndef __CUSTOM_SPACE_H__
#define __CUSTOM_SPACE_H__

#include <autoconf.h>

#ifdef CONFIG_ENABLE_CLOSE_CMD_RECOGNIZE
# include "close_recognize/lvp_close_recognize.h"
#endif
#ifdef CONFIG_ENABLE_STORAGE_CUSTOM_DATA
# include "write_custom_data/lvp_write_custom_data.h"
#endif

#define CUSTOM_TO_XIP_ADDR(addr)    (unsigned char*)(CONFIG_FLASH_XIP_BASE + (unsigned int)(addr))
#define FLASH_BLOCK_SIZE            (4096)
#define CUSTOM_STORAGE_SPACE_LEN    (CONFIG_CUSTOM_STORAGE_SPACE_LEN * FLASH_BLOCK_SIZE)
#define CUSTOM_STORAGE_SPACE_ADDR   (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH)

#if defined(CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM) && (CONFIG_NEED_WRITE_FLASH_CLOSE_KV_NUM != 0)
# define CLOSE_RECOGNIZE_ADDR       (CUSTOM_STORAGE_SPACE_ADDR)
# define CLOSE_RECOGNIZE_LEN        (FLASH_BLOCK_SIZE)
# define WRITE_CUSTOM_DATA_ADDR     (CUSTOM_STORAGE_SPACE_ADDR + FLASH_BLOCK_SIZE)
# define WRITE_CUSTOM_DATA_LEN      (CUSTOM_STORAGE_SPACE_LEN - CLOSE_RECOGNIZE_LEN)
#else
# define WRITE_CUSTOM_DATA_ADDR     (CUSTOM_STORAGE_SPACE_ADDR)
# define WRITE_CUSTOM_DATA_LEN      (CUSTOM_STORAGE_SPACE_LEN)
#endif

// 使用示例

/*
#ifdef CONFIG_ENABLE_STORAGE_CUSTOM_DATA
# include <lvp_custom_space.h>
#endif

#ifdef CONFIG_ENABLE_STORAGE_CUSTOM_DATA
typedef struct {
    int cur_mode;         // 记录最后设置的场景模式
    int cur_light_state;  // 记录最后灯的状态
    int cur_brightness;   // 记录当前的亮度,不包括关灯状态
    int volume;           // 记录音量
} __attribute__((packed)) GongNiuSt;

static GongNiuSt s_gongniu_st = {
    .cur_mode = 0x3,
    .cur_light_state = 0x2,
    .cur_brightness = 0x5,
    .volume  = 50,
};
#endif

static int Sample(void)
{
#ifdef CONFIG_ENABLE_CLOSE_CMD_RECOGNIZE
    // 初始化
    LvpInitCloseCmdRecognize();

    // 关闭 kv:100,掉电不记忆
    LvpCloseCmdRecognize(100, 0);

    // 关闭 kv:101,掉电记忆
    LvpCloseCmdRecognize(101, 1);

    // 打开 kv: 101 识别
    LvpOpenCmdRecognize(101);

    int close_array[] = {102, 103, 333, 444};

    // 将 close_array 中所有指令词关闭，掉电不记忆
    LvpBatchProcesCmdRecognize(close_array, 4, 0);

    // 将 close_array 中所有指令词关闭，掉电记忆
    LvpBatchProcesCmdRecognize(close_array, 4, 1);

    // 重置所有的指令词
    LvpClearAllCloseInfo();
#endif

#ifdef CONFIG_ENABLE_STORAGE_CUSTOM_DATA
    // 将自定义数据写入flash
    if (LvpWriteCustomData((unsigned char*)&s_gongniu_st, sizeof(s_gongniu_st)) != 0) {
        printf("write error\n");
    }

    // 获取之前写入flash的数据长度与数据存储地址
    int data_len = 0;
    GongNiuSt *read_st = (GongNiuSt *)LvpReadCustomData(&data_len);


    memset(&s_gongniu_st, 0, sizeof(GongNiuSt));
    // 将之前写入flash的数据内容拷贝到
    memcpy((void*)&s_gongniu_st, LvpGetCustomDataAddr(), sizeof(GongNiuSt));
#endif

    return 0;
}
 */

#endif // __CUSTOM_SPACE_H__
