# If we are called by kernel build system
ifneq ($(KERNELRELEASE),)
	obj-m := gpio_kernel_driver.o
# If we are called from the command line, we call the kernel build system
else
	KERNELDIR ?= /home/fdeslandes/DaisyWheel/linux-3.10.10
	PWD:=$(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
