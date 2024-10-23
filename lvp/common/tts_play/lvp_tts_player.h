/* LVP
 * Copyright (C) 2001-2023 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 *tts_player.h
 *
 */

#ifndef __TTS_PLAYER_H__
#define __TTS_PLAYER_H__

#include <autoconf.h>
#include <base_addr.h>
#include <lvp_app.h>

#ifdef CONFIG_CUSTOM_STORAGE_SPACE
# define TTS_BIN_FLASH_ADDR  (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH + CONFIG_CUSTOM_STORAGE_SPACE_LEN * 4096)
#else
# define TTS_BIN_FLASH_ADDR  (CONFIG_RESOURCE_BIN_ADDRESS_IN_FLASH)
#endif
#define TO_XIP_ADDR(addr)   (unsigned char*)(CONFIG_FLASH_XIP_BASE + (unsigned int)addr)

typedef struct {
    int             kws_id;
    int             tts_num;
    unsigned int    offset;
    unsigned int    length;
} TTS_INFO;

typedef struct {
    int list_num;
    TTS_INFO *tts_info;
} TTS_LIST_INFO;

int LvpGetTtsInfoStartAddr(void);
int LvpGetTtsInfoLength(void);
int LvpTtsPlayerInit(void);
int LvpTtsPlayerSuspend(int type);
int LvpTtsPlayerResume(void);
int LvpTtsPlayerDone(void);
int LvpTtsPlayerResponse(APP_EVENT *app_event);

#endif /* __TTS_PLAYER_H__ */
