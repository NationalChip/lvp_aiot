#
# Voice Signal Preprocess
# Copyright (C) 2001-2023 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: lvp/common source list
#
#=================================================================================#

ifeq ($(CONFIG_LVP_ENABLE_KEYWORD_RECOGNITION), y)
-include lvp/vui/kws/Makefile
vui_objs += $(kws_objs)
vui_objs += $(gxdecoder_objs)
endif



