obj-m += misc.o

org := /lib/modules/$(shell uname -r)/build
x86_mod := /home/scl/learning/linux


all:
	make -C $(x86_mod) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

