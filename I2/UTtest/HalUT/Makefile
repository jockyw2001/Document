TARGET := halut
OUT_DIR := ./out

INCLUDE_HEAD := -IHalUT
INCLUDE_ALKAID_HEAD := $(INCLUDE_HEAD)/ALKAID

EXTRA_CFLAGS   +=   $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/divp \
					$(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/cmdq_service \
					$(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/common
EXTRA_CFLAGS   += $(INCLUDE_HEAD)/HT_MODULE/inc	\
					$(INCLUDE_HEAD)/HT_COMMON/inc

ifeq (1, $(MOD_MHAL_CMDQ))
$(TARGET)-objs += HT_HAL/mhal_cmdq.o
endif

ifeq (1, $(USE_MI_SYS))
EXTRA_CFLAGS += -DUSE_MI_SYS
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/interface/include/common
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/interface/include/sys
endif

ifneq (0, $(MOD_DIVP))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/divp \
				$(INCLUDE_HEAD)/HT_DIVP/inc
EXTRA_CFLAGS += -DMOD_DIVP
$(TARGET)-objs += HT_DIVP/src/ht_divp.o
ifeq (2, $(MOD_DIVP))
$(TARGET)-objs += HT_HAL/mhal_divp.o
endif
endif

ifneq (0, $(MOD_VPE))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/vpe \
				$(INCLUDE_HEAD)/HT_VPE/inc
EXTRA_CFLAGS += -DMOD_VPE
$(TARGET)-objs += HT_VPE/src/ht_vpe.o
ifeq (2, $(MOD_VPE))
$(TARGET)-objs += HT_HAL/mhal_vpe.o
endif
endif

ifneq (0, $(MOD_VIF))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/vif \
				$(INCLUDE_HEAD)/HT_VIF/inc
EXTRA_CFLAGS += -DMOD_VIF
$(TARGET)-objs += HT_VIF/src/ht_vif.o
ifeq (2, $(MOD_VIF))
$(TARGET)-objs += HT_HAL/mhal_vif.o
endif
endif

ifneq (0, $(MOD_AI))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/aio \
				$(INCLUDE_HEAD)/HT_AUDIO/inc
EXTRA_CFLAGS += -DMOD_AI
$(TARGET)-objs += HT_AUDIO/src/ht_ai.o
ifeq (2, $(MOD_AI))
$(TARGET)-objs += HT_HAL/mhal_audio.o
endif
endif

ifneq (0, $(MOD_AO))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/aio \
				$(INCLUDE_HEAD)/HT_AUDIO/inc
EXTRA_CFLAGS += -DMOD_AO
$(TARGET)-objs += HT_AUDIO/src/ht_ao.o
ifeq (2, $(MOD_AO))
$(TARGET)-objs += HT_HAL/mhal_audio.o
endif
endif

ifneq (0, $(MOD_HDMI))
EXTRA_CFLAGS += $(INCLUDE_HEAD)/HT_HDMI/inc
EXTRA_CFLAGS += -DMOD_HDMI
$(TARGET)-objs += HT_HDMI/src/ht_hdmi.o
endif

ifneq (0, $(MOD_DISP))
EXTRA_CFLAGS += $(INCLUDE_ALKAID_HEAD)/sdk/mhal/include/disp \
				$(INCLUDE_HEAD)/HT_DISP/inc
EXTRA_CFLAGS += -DMOD_DISP
$(TARGET)-objs += HT_DISP/src/ht_disp.o
endif


ifeq (1, $(MOD_VDEC))
EXTRA_CFLAGS += -DMOD_VDEC
EXTRA_CFLAGS += $(INCLUDE_HEAD)/HT_VDEC/inc
$(TARGET)-objs += HT_VDEC/src/ht_vdec.o
endif

ifeq (1, $(MOD_MHAL_GFX))
EXTRA_CFLAGS += -DUSE_GE
$(TARGET)-objs += HT_HAL/mhal_gfx.o
endif

ifeq (1, $(WRITE_DDR))
EXTRA_CFLAGS += -DHT_WRITE_DDR
endif

ifeq (1, $(MOD_UTPA))
EXTRA_CFLAGS += -DMSOS_TYPE_LINUX_KERNEL
ifeq (i2, $(CHIP_NAME))
EXTRA_CFLAGS += -DCHIP_I2
EXTRA_CFLAGS   +=   $(INCLUDE_ALKAID_HEAD)/sdk/mhal/i2/utpa/mxlib/include
else
ifeq (k6l, $(CHIP_NAME))
EXTRA_CFLAGS   +=   $(INCLUDE_ALKAID_HEAD)/sdk/mhal/k6l/utpa/
endif
endif
endif

ifeq (i2, $(CHIP_NAME))
export CROSS_COMPILE=arm-linux-gnueabihf-
KDIR=$(I2_KERNEL)
else
ifeq (k6l, $(CHIP_NAME))
export CROSS_COMPILE=arm-none-linux-gnueabi-
KDIR=$(K6L_KERNEL)
endif
endif

obj-m := $(TARGET).o
$(TARGET)-objs += HT_MODULE/src/ht_module.o HT_COMMON/src/ht_common.o

export ARCH=arm

CC  = $(CROSS_COMPILE)gcc
PWD := $(shell pwd)

.PHONY : all create clean module

all:create module
	@cp *.ko $(OUT_DIR)
module:
	@make modules -C $(KDIR) M=$(PWD)
create:
ifneq ($(KDIR)/HalUT, $(wildcard $(KDIR)/HalUT))
	@ln -s $(shell pwd) $(KDIR)/HalUT
endif
ifneq (./ALKAID , $(wildcard ./ALKAID))
	@ln -s $(ALKAID_PATH) ./ALKAID
endif
	@mkdir -p $(OUT_DIR)

clean:
ifeq ($(KDIR)/HalUT, $(wildcard $(KDIR)/HalUT))
	@rm -rf $(KDIR)/HalUT
endif
ifeq (./ALKAID , $(wildcard ./ALKAID))
	@rm -rvf ./ALKAID
endif
	@make -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUT_DIR) *.yuv *.o
	@rm -rf ./output
