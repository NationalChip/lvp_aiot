#
# LVP
# Copyright (C) 2001-2023 NationalChip Co., Ltd
# ALL RIGHTS RESERVED!
#
# peripheral.mk: source list of peripherals
#
#=================================================================================#

INCLUDE_DIR += lvp/common/peripherals/
INCLUDE_DIR += lvp/common/peripherals/gpio_led/
INCLUDE_DIR += lvp/common/peripherals/pwm_led/
INCLUDE_DIR += lvp/common/peripherals/slight_led/

dev_objs += lvp/common/peripherals/gpio_led/gpio_led.o
dev_objs += lvp/common/peripherals/pwm_led/pwm_led.o
dev_objs += lvp/common/peripherals/pwm_motor/pwm_motor.o
dev_objs += lvp/common/peripherals/slight_led/slight_led.o

INCLUDE_DIR += lvp/common/peripherals/multi_button/src/
dev_objs += lvp/common/peripherals/multi_button/src/multi_button.o
dev_objs += lvp/common/peripherals/multi_button/src/button_simulate.o

