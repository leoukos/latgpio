# If we are called by kernel build system
ifneq ($(KERNELRELEASE),)
	obj-m := irq_ft_gpio_handler.o
# If we are called from the command line, we call the kernel build system
else
	KERNELDIR ?= /home/fdeslandes/Tarballs/RPI/linux
	PWD:=$(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
