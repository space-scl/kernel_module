obj-m += misc.o

org := /lib/modules/$(shell uname -r)/build
x86_mod := /home/scl/learning/linux



arm_mod := /home/scl/learning/linux
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-


all:
	make -C $(arm_mod) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	#make -C $(org) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

