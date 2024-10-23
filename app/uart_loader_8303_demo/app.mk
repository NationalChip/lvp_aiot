#
# Voice Signal Preprocess
# Copyright (C) 2001-2021 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# Makefile: source list of VSP other hook
#
#=================================================================================#

ifeq ($(CONFIG_LVP_APP_UART_LOADER_8303), y)

app_objs += app/uart_loader_8303_demo/uart_loader_8303_demo.o

ifeq ($(CONFIG_APP_UART_LOADER_HAS_LED), y)
app_objs += app/self_learn_demo/vc_led.o
endif

endif
