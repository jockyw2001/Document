obj-m := hello.o
UTDIR:= $(SHELL pwd)/../../
KDIR := $(shell pwd)/../../../../../../../ksc/linux-3.18_I3
DIR  := $(shell pwd)
default: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf .*.cmd *.o *.mod.c *.ko .tmp_versions *.order *symvers *Module.markers 
