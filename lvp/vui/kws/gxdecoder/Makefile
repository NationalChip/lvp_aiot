GXDECODER_DIR_PRE  = lvp/vui/kws/gxdecoder

-include $(GXDECODER_DIR_PRE)/model_fst/*/Makefile
-include $(GXDECODER_DIR_PRE)/src/Makefile
-include $(GXDECODER_DIR_PRE)/utility/Makefile

INCLUDE_DIR    += $(GXDECODER_DIR_PRE)/include

kws_objs       += $(GXDECODER_DIR_PRE)/gxdecoder.o

ifeq ($(CONFIG_ENABLE_USED_FOR_ALL_GRUS_FAMILY), y)
ifeq ($(CONFIG_MCU_ENABLE_PRINTF), y)
build: lib/libgxdecoder_all_debug_v1.0.8.a
lib/libgxdecoder_all_debug_v1.0.8.a:$(gxdecoder_objs)
	@mkdir -p lib/
	@echo [$(AR) archiving $@]
	@$(AR) $(ARFLAGS) $@ $^
else
build: lib/libgxdecoder_all_release_v1.0.8.a
lib/libgxdecoder_all_release_v1.0.8.a:$(gxdecoder_objs)
	@mkdir -p lib/
	@echo [$(AR) archiving $@]
	@$(AR) $(ARFLAGS) $@ $^
endif
else
ifeq ($(CONFIG_MCU_ENABLE_PRINTF), y)
build: lib/libgxdecoder_debug_v1.0.8.a
lib/libgxdecoder_debug_v1.0.8.a:$(gxdecoder_objs)
	@mkdir -p lib/
	@echo [$(AR) archiving $@]
	@$(AR) $(ARFLAGS) $@ $^
else
build: lib/libgxdecoder_release_v1.0.8.a
lib/libgxdecoder_release_v1.0.8.a:$(gxdecoder_objs)
	@mkdir -p lib/
	@echo [$(AR) archiving $@]
	@$(AR) $(ARFLAGS) $@ $^
endif
endif