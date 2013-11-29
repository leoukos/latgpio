USR=userspace
KERN=kernel

all:
	make -C $(USR) 
	make -C $(KERN) 

clean:
	make -C $(USR) clean
	make -C $(KERN) clean
