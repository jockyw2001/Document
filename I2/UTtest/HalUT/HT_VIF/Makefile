
#UT_VIF
obj-m=UTvif.o
UTvif-objs=src/ht_vif.o src/HAL_VIF.o src/ht_common.o
export ARCH=arm
export CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabi-

KDIR=/home/wenshuai.xi/kernel
CC= $(CROSS_COMPILE)gcc

PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	@rm -rf .*.*.cmd *.o *.mod.c *.mod.o *.ko *.cmd modules.order Module.symvers .tmp_versions src/.*.*.cmd src/*.o





