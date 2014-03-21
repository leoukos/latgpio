ifneq (${KERNELRELEASE},)
	obj-m += irq_ft_gpio_handler.o hello_world.o
else
	ARCH ?= arm
	KERNEL_DIR ?= /home/fdeslandes/Tests/igepv2/fudge/linux-3.0.4
	CROSS_COMPILE ?= /home/fdeslandes/Tests/buildroot-glibc-2013.11/output/host/usr/bin/arm-buildroot-linux-gnueabi-
	MODULE_DIR := $(shell pwd)

all: modules

modules:
	${MAKE} -C ${KERNEL_DIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} SUBDIRS=${MODULE_DIR} modules

clean:
	rm -f *.o .*.o .*.o.* *.ko .*.ko *.mod.* .*.mod.* .*.cmd
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions

endif
