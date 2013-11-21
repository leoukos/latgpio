CC=gcc
CFLAGS=-g -Wall -Werror
LIBS=-lrt

userspace: gpio_userspace_driver.o
	$(CC) $(CFLAGS) $(LIBS) gpio_userspace_driver.o -o gpio_userspace_driver

gpio_userspace_driver.o: gpio_userspace_driver.c
	$(CC) $(CFLAGS) -c gpio_userspace_driver.c

clean:
	rm *.o gpio_userspace_driver
