3rdparty_mkfile := $(abspath $(lastword $(MAKEFILE_LIST)))
3rdparty_path := $(patsubst %/, %, $(dir $(3rdparty_mkfile)))

ifeq ($(CONFIG_USE_OPUS), y)
-include $(3rdparty_path)/opus/Makefile
endif

-include $(3rdparty_path)/adpcm/Makefile

