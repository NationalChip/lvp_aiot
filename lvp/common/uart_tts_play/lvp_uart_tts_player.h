/* LVP
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 *tts_player.h
 *
 */

#ifndef __UART_TTS_PLAY_H__
#define __UART_TTS_PLAY_H__

#include <autoconf.h>
#include <base_addr.h>

#include "tts_play/lvp_tts_player.h"

#define REPLY_PLAYER_UART_CMD_DATA_LEN  CONFIG_REPLY_PLAYER_UART_CMD_DATA_LEN
#define REPLY_PLAYER_UART               CONFIG_REPLY_PLAYER_UART
#define REPLY_PLAYER_UART_BAUDRATE      CONFIG_REPLY_PLAYER_UART_BAUDRATE

typedef struct {
    unsigned char   uart_data[REPLY_PLAYER_UART_CMD_DATA_LEN];
    int             event_id;
} __attribute__((packed)) UART_INFO;

typedef struct {
    int list_num;
    UART_INFO *uart_info;
} UART_LIST_INFO;

int LvpGetUartTtsInfoLength(void);
/*
 * @brief 初始化串口被动播报功能。
 *        注意当其它地方使用了相同的串口时，需要重新调用此接口进行初始化。
 */
int LvpUartTtsPlayerInit(void);
int LvpUartTtsPlayerSuspend(int type);
int LvpUartTtsPlayerResume(void);
int LvpUartTtsPlayerDone(void);
int LvpUartTtsPlayerTick(void);

#endif /* __UART_TTS_PLAY_H__ */
