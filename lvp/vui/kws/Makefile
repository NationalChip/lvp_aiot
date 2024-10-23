#
# Voice Signal Preprocess
# Copyright (C) 2001-2019 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: source list in dsp/vpa/arach_lib/src/ctc/decoder
#
#=================================================================================#

KWS_DIR_PRE  = lvp/vui/kws



ifeq ($(CONFIG_LVP_ENABLE_USER_DECODER), y)
kws_objs       += $(KWS_DIR_PRE)/user_decoder.o
else

ifeq ($(CONFIG_LVP_ENABLE_CTC_GX_DECODER), y)
kws_objs       += $(KWS_DIR_PRE)/kws_strategy.o
kws_objs       += $(KWS_DIR_PRE)/ctc_gxdecoder.o
-include $(KWS_DIR_PRE)/gxdecoder/Makefile
endif
endif


# models
-include $(KWS_DIR_PRE)/models/*/*/Makefile

ifeq ($(CONFIG_ENABLE_USED_FOR_ALL_GRUS_FAMILY), y)
ifeq ($(CONFIG_MCU_ENABLE_PRINTF), y)
LIBS           += -lgxdecoder_all_debug_v1.0.8
else
LIBS           += -lgxdecoder_all_release_v1.0.8
endif
else
ifeq ($(CONFIG_MCU_ENABLE_PRINTF), y)
LIBS           += -lgxdecoder_debug_v1.0.8
else
LIBS           += -lgxdecoder_release_v1.0.8
endif
endif

ifeq ($(CONFIG_LVP_SELF_LEARNING_ENABLE), y)
kws_objs       += $(KWS_DIR_PRE)/self_learning.o
endif
