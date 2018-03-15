#obj-m += ht_test.o         
obj-m := haluttest.o
haluttest-objs = ht_common.o vpe/ht_vpe.o ht_test.o

export ARCH=arm

#I3
#export CROSS_COMPILE=arm-linux-gnueabihf-
#KDIR = /home/koda.xu/test2/linux-3.18

#K6L
export CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabi-
KDIR = $(PWD)/../kernel

#KDIR := /lib/modules/`uname -r`/build
CC  = $(CROSS_COMPILE)gcc
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	#$(CROSS_COMPILE)strip -g halut_test.ko
clean:
	@rm -rf .*.*.cmd *.o *.mod.c *.mod.o *.ko *.cmd modules.order Module.symvers .tmp_versions

