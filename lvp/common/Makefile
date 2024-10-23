#
# Voice Signal Preprocess
# Copyright (C) 2001-2023 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: lvp/common source list
#
#=================================================================================#
-include lvp/common/snpu_engine/Makefile

common_objs +=lvp/common/lvp_buffer.o
common_objs += lvp/common/lvp_queue.o
common_objs += lvp/common/lvp_audio_in.o
common_objs += lvp/common/lvp_i2c_msg.o
common_objs += lvp/common/lvp_pmu.o
common_objs += lvp/common/lvp_system_init.o
common_objs += lvp/common/lvp_audio_out.o

ifeq ($(CONFIG_LVP_HAS_UART_MESSAGE_2_0), y)
common_objs += lvp/common/uart_message_v2.o
endif

ifeq ($(CONFIG_LVP_HAS_UART_RECORD), y)
common_objs += lvp/common/lvp_uart_record.o
endif

ifeq ($(CONFIG_LVP_HAS_VOICE_PLAYER), y)
-include lvp/common/player/voice_player/player.mk
common_objs += $(player_objs)
endif

ifeq ($(CONFIG_LVP_FACTORY_MIC_TEST), y)
common_objs += lvp/common/lvp_mic_test.o
endif

ifeq ($(CONFIG_ENABLE_CACULATE_STANDBY_RATIO), y)
common_objs += lvp/common/lvp_standby_ratio.o
endif

ifeq ($(CONFIG_LVP_HAS_TTS_PLAYER), y)
-include lvp/common/tts_play/Makefile
common_objs += $(tts_play_objs)
ifeq ($(CONFIG_LVP_HAS_UART_TTS_REPLY_PLAYER), y)
-include lvp/common/uart_tts_play/Makefile
common_objs += $(uart_play_objs)
endif
endif

ifeq ($(CONFIG_LVP_HAS_KWS_UART_REPORT), y)
-include lvp/common/kws_uart_report/Makefile
common_objs += $(kws_uart_report_objs)
endif

ifeq ($(CONFIG_CUSTOM_STORAGE_SPACE), y)
-include lvp/common/custom_space/Makefile
common_objs += $(custom_space_objs)
endif

ifeq ($(CONFIG_LVP_HAS_UART_LOADER), y)
-include lvp/common/uart_loader/Makefile
common_objs += $(uart_loader_objs)
endif

include lvp/common/peripherals/peripherals.mk
