obj-m := tsoenabler.o
ccflags-y := -Wno-declaration-after-statement
PWD := $(shell pwd)
KERNEL:=/home/${USER}/AsahiLinux/linux/linux
all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -C $(KERNEL) M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod* *.order *.symvers
