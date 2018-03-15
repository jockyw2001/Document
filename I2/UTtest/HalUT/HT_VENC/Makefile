KERN_DIR = /home/black.lian/linux3.18/linux-3.18/
OUT_DIR = ../out_file
RM= rm -f

#obj-m表示将module_test.o编译成单独的模块
obj-m += venc_test03.o

#下面这句真正开始编译
all:
	make -C $(KERN_DIR) M=`pwd` modules 

.PHONY: clean   
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	$(RM) $(OUT_DIR)/*
