#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

/**
 * Opens a file and write to it
 *
 * file		:	filename
 * buf		:	buffer
 * size		:	buffer size
 * return	:	-1 if opening fails
 * 				-2 if write fails
 */
int open_and_write(char* file, char* buf, size_t size)
{
	FILE* fd;

	if((fd=fopen(file, "w"))== NULL){
		return -1;
	}

	if(fwrite(buf, size, 1, fd) <= 0 ){
		return -2;
	}

	fclose(fd);
	return 0;
}

/**
 * Exports gpio, set direction and edge
 *
 * return	:	-1 if export fails
 * 				-2 if direction fails
 * 				-3 if edge fails
 */
int gpio_init(char* edge, char* direction, char* gpio_no)
{
	char buf[128];

	/* Check presence */

	/* Export gpio */
	if(open_and_write("gpio/export", gpio_no, sizeof(char)*strnlen(gpio_no, 8)) < 0){
		fprintf(stderr, "Could not export gpio %s\n", gpio_no);
		return -1;
	}

	/* Set direction */
	if(snprintf(buf, 128, "gpio/gpio%s/direction", gpio_no) < 0){
		fprintf(stderr, "Could not create direction string for gpio %s\n", gpio_no);
		return -2;
	}
	if(open_and_write(buf, direction, sizeof(char)*strnlen(direction, 3)) < 0){
		fprintf(stderr, "Could not set direction for gpio %s\n", gpio_no);
		return -2;
	}

	/* Set edge */
	if(snprintf(buf, 128, "gpio/gpio%s/edge", gpio_no) < 0){
		fprintf(stderr, "Could not create edge string for gpio %s\n", gpio_no);
		return -3;
	}
	if(open_and_write(buf, edge, sizeof(char)*strnlen(edge, 8)) < 0){
		fprintf(stderr, "Could not create edge string for gpio %s\n", gpio_no);
		return -1;
	}

	return 0;
}

/**
 * Unexport gpio
 *
 */
int gpio_clean(char* gpio_no)
{
	/* Unexport gpio_no */
	if(open_and_write("gpio/unexport", gpio_no, sizeof(char)*strnlen(gpio_no, 8)) < 0){
		fprintf(stderr, "Could not unexport gpio %s\n", gpio_no);
		return -1;
	}
	return 0;
}

int gpio_handle(int gpio_no)
{
	int gpio_fd;
	fd_set gpio_fds;
	struct timeval tv;
	char buffer[2];
	char gpio_value_file[128];


	
	/* Open the /sys file */
	if(snprintf(gpio_value_file, 128, "/sys/class/gpio/gpio%d/value", gpio_no) < 0){
		fprintf(stderr, "An error occured while opening value file\n");
		exit(EXIT_FAILURE);
	}
	if((gpio_fd=open(gpio_value_file, O_RDONLY)) < 0){
		fprintf(stderr, "Coudl not open %s\n", gpio_value_file);
		exit(EXIT_FAILURE);
	}

	/* Main loop */
	while(1) {
		/* prepare event table */
		FD_ZERO(&gpio_fds);
		FD_SET(gpio_fd, &gpio_fds);

		/* Sleep untill event */
		if(select(gpio_fd+1, NULL, NULL, &gpio_fds, NULL) <0){
			fprintf(stderr, "Select failed\n");
			break;
		}
		
		gettimeofday(&tv, NULL);

		/* Return to the beginning of the file */
		lseek(gpio_fd, 0, SEEK_SET);

		/* Read the value */
		if(read(gpio_fd, &buffer, 2)!=2){
			fprintf(stderr, "Could not read the value\n");
			break;
		}

		/* Get read of final \n */
		buffer[1]='\0';

		/* Print value */
		fprintf(stdout, "[%ld.%06ld]: %s\n", tv.tv_sec, tv.tv_usec, buffer);
	}

	/* We should never get here  but still... */
	close(gpio_fd);
	return(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	/* Variables */
	char gpio_no[8];
	char edge[8];
	int ret;

	/* Command line arguments */

	/* Gpio initialisation */
	ret=gpio_init(argv[1], "out",  argv[2]);
	if(ret == -1){
		fprintf(stderr, "Could not export gpio %s\n", gpio_no);
		return EXIT_FAILURE;
	} else if (ret == -2 || ret == -3) {
		fprintf(stderr, "Could not set direction and/or edge for gpio %s\n", gpio_no);
		gpio_clean(gpio_no);
	}

	/* Handle interrupt */
	//gpio_handle(gpio_no);

	/* Gpio clean */		
	gpio_clean(gpio_no);

	return(EXIT_SUCCESS);
}


