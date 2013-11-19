#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

int gpio_init(char* edge, char* direction, int gpio_no)
{
	int fd;
	char file[128];

	/* Check presence of gpio  */

	/* Export gpio */
	if((fd=open("/sys/class/gpio/export", O_WRONLY)) < 0){
		fprintf(stderr, "Could not export gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}
	if(write(fd, &gpio_no, sizeof(int))==-1){
		fprintf(stderr, "Could not export gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}

	close(fd);

	/* Set edge */
	if(snprintf(file, 128, "/sys/class/gpio/gpio%d/edge", gpio_no) < 0){
		fprintf(stderr, "An error occured while opening value file\n");
		exit(EXIT_FAILURE);
	}

	if((fd=open(file, O_WRONLY)) < 0){
		fprintf(stderr, "Could not set edge for gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}
	if(write(fd, &edge, sizeof(char)*strnlen(edge, 8))==-1){
		fprintf(stderr, "Could not set edge for gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}

	close(fd);
	
	/* Set direction */
	if(snprintf(file, 128, "/sys/class/gpio/gpio%d/direction", gpio_no) < 0){
		fprintf(stderr, "An error occured while opening direction file\n");
		exit(EXIT_FAILURE);
	}

	if((fd=open(file, O_WRONLY)) < 0){
		fprintf(stderr, "Could not set direction for gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}
	if(write(fd, &direction, sizeof(char)*strnlen(direction, 4))==-1){
		fprintf(stderr, "Could not set direction for gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}

	close(fd);

	return 0;
}

int gpio_clean(int gpio_no)
{
	int fd;

	/* Unexport gpio */
	if((fd=open("/sys/class/gpio/unexport", O_WRONLY)) < 0){
		fprintf(stderr, "Could not unexport gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}
	if(write(fd, &gpio_no, sizeof(int))==-1){
		fprintf(stderr, "Could not unexport gpio %d\n", gpio_no);	
		exit(EXIT_FAILURE);
	}

	close(fd);
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
	int gpio_no;
	char edge[8];

	/* Command line arguments */
	if(argc != 3){
		fprintf(stderr, "Usage : %s  <edge>  <gpio_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if((strncmp(argv[1], "none", 4)==0) ||
		(strncmp(argv[1], "falling", 7)==0) ||
		(strncmp(argv[1], "rising", 6)==0) ||
		(strncmp(argv[1], "both", 4)==0)){
		strncpy(edge, argv[1], 8);
	} else{
		fprintf(stderr, "Wrong edge value\n");
		exit(EXIT_FAILURE);
	}

	if(sscanf(argv[2], "%d", &gpio_no)!=1){
		fprintf(stderr, "Could not read gpio number\n");
		exit(EXIT_FAILURE);
	}

	/* Gpio initialisation */
	gpio_init(edge, "out",  gpio_no);

	/* Handle interrupt */
	gpio_handle(gpio_no);

	/* Gpio clean */		
	gpio_clean(gpio_no);

	return(EXIT_SUCCESS);
}


