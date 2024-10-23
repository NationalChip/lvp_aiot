#
# Voice Signal Preprocess
# Copyright (C) 2001-2021 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: source list of VSP other hook
#
#=================================================================================#

ifeq ($(CONFIG_LVP_APP_SLD_DEMO), y)
app_objs += app/self_learn_demo/self_learn_demo.o
ifeq ($(or $(findstring y,$(CONFIG_APP_SLD_APP_VC_HAS_LED)),$(findstring y,$(CONFIG_LVP_SELF_LEARN_UES_LED_TIP))),y)
app_objs += app/self_learn_demo/vc_led.o
endif
ifeq ($(CONFIG_APP_SLD_GX_STANDARD_SERIAL_UART_PROTOCOL), y)
app_objs += app/self_learn_demo/vc_message.o
endif
endif
