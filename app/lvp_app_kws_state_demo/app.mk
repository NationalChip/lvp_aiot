#
# Voice Signal Preprocess
# Copyright (C) 2001-2021 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: source list of VSP other hook
#
#=================================================================================#

ifeq ($(CONFIG_LVP_APP_KWS_STATE_DEMO), y)
app_objs += app/lvp_app_kws_state_demo/lvp_app_kws_state_demo.o
ifeq ($(CONFIG_APP_VC_HAS_LED), y)
app_objs += app/lvp_app_kws_state_demo/vc_led.o
endif
ifeq ($(CONFIG_GX_STANDARD_SERIAL_UART_PROTOCOL), y)
app_objs += app/lvp_app_kws_state_demo/vc_message.o
endif
endif
