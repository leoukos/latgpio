USR=userspace
KERN=kernel
CROSS_COMPILE=
ARCH=

all:
	make -C $(USR) 
	make -C $(KERN) 

clean:
	make -C $(USR) clean
	make -C $(KERN) clean
