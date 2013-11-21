#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <getopt.h>

#define TYPE_HANDLER 0
#define TYPE_TRIGGER 1

#define GPIO_LOW '0'
#define GPIO_HIGH '1'

#define GPIO_OUT "out"
#define GPIO_IN "in"

#define GPIO_NONE_EDGE "none"
#define GPIO_RISING_EDGE "rising"
#define GPIO_FALLING_EDGE "falling"
#define GPIO_BOTH_EDGE "both"

/**
 * Opens a file and write to it
 *
 * file		:	filename
 * buf		:	buffer
 * size		:	buffer size
 * return	:	-1 if opening fails
 * 				-2 if write fails
 */
int open_and_write(const char* file, const char* buf, const size_t size)
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
int gpio_init(const char* edge, const char* direction, const char* gpio_no)
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
int gpio_clean(const char* gpio_no)
{
	/* Unexport gpio_no */
	if(open_and_write("gpio/unexport", gpio_no, sizeof(char)*strnlen(gpio_no, 8)) < 0){
		fprintf(stderr, "Could not unexport gpio %s\n", gpio_no);
		return -1;
	}
	return 0;
}

int gpio_handle(const char* gpio_in, const char* gpio_out)
{
	int gpio_out_fd, gpio_in_fd;
	fd_set gpio_in_fds;
	char value;
	char gpio_input_file[128];
	char gpio_output_file[128];

	
	/* Open the input file */
	if(snprintf(gpio_input_file, 128, "gpio/gpio%s/value", gpio_in) < 0){
		fprintf(stderr, "An error occured while opening value file for input gpio %s\n", gpio_in);
		return EXIT_FAILURE;
	}
	if((gpio_in_fd=open(gpio_input_file, O_RDONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_input_file);
		return EXIT_FAILURE;
	}

	/* Open the output file */
	if(snprintf(gpio_output_file, 128, "gpio/gpio%s/value", gpio_out) < 0){
		fprintf(stderr, "An error occured while opening value file for output gpio %s\n", gpio_out);
		return EXIT_FAILURE;
	}
	if((gpio_out_fd=open(gpio_output_file, O_WRONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_output_file);
		return EXIT_FAILURE;
	}

	value = GPIO_HIGH;
	if(write(gpio_out_fd, &value, 1)!=1){
		fprintf(stderr, "Could not write the value %c to %s\n", value, gpio_output_file);
	}

	/* Main loop */
	while(1) {
		/* prepare event table */
		FD_ZERO(&gpio_in_fds);
		FD_SET(gpio_in_fd, &gpio_in_fds);

		/* Sleep untill event, no timeout */
		if(select(gpio_in_fd+1, NULL, NULL, &gpio_in_fds, NULL) <0){
			fprintf(stderr, "Select failed\n");
			break;
		}
		
		/* Return to the beginning of the output file */
		lseek(gpio_out_fd, 0, SEEK_SET);


		/* Reverse the value */
		value = (value == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW;
		if(write(gpio_out_fd, &value, 1)!=1){
			fprintf(stderr, "Could not write the value\n");
			break;
		}
	}

	/* We should never get here but still... */
	close(gpio_in_fd);
	close(gpio_out_fd);
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	/* Variables */
	char gpio_in[8], gpio_out[8];
	char edge[8]="rising";
	int ret;
	int opt_flag = 0; /* will test if all the values are set */
	int type = TYPE_HANDLER; /* defines if the tigger or handler mode should be set */

	/* Command line arguments */
	char* usage = "\t-ioeph --triger --handler";
	int option;
	int longindex;
	char* optstring = "i:o:e:p:h";
	char* error_opt = "Wrong option";
	struct option longopts[] = {
		{"in",			1,		NULL,		'i'},
		{"out",			1,		NULL,		'o'},
		{"edge",			1,		NULL,		'e'},
		{"period",		1,		NULL,		'p'},
		{"help",			0,		NULL,		'h'},
		{"handler",		0,		NULL,		'1'},
		{"trigger",		0,		NULL,		'2'},
		{NULL,			0,		NULL,		0},
		};

	while( (option=getopt_long(argc, argv, optstring, longopts, &longindex)) != -1){
		switch(option){
			case 'i':
				if(sscanf(optarg, "%s", &gpio_in) != 1){
					fprintf(stderr, "Could not set gpio input\n");
					return EXIT_FAILURE;
				}
				opt_flag= opt_flag | 1;
				break;
			case 'o':
				if(sscanf(optarg, "%s", &gpio_out) != 1){
					fprintf(stderr, "Could not set gpio output\n");
					return EXIT_FAILURE;
				}
				opt_flag= (opt_flag & 1) | 2;
				break;
			case 'e':
				if(strncmp(optarg, GPIO_NONE_EDGE, 4) == 0 ||
					strncmp(optarg, GPIO_RISING_EDGE, 6) == 0 ||
					strncmp(optarg, GPIO_FALLING_EDGE, 7) == 0 ||
					strncmp(optarg, GPIO_BOTH_EDGE, 6) == 0){
					strncpy(edge, optarg, 7);
				} else {
					strncpy(edge, GPIO_RISING_EDGE, 6);
				}
				break;
			case '1':
				type=TYPE_HANDLER;
				break;
			case '2':
				type=TYPE_TRIGGER;
				break;
			case 'p':
				break;
			case 'h':
				fprintf(stderr, "Usage : \n%s\n", usage);
				return EXIT_FAILURE;
				break;
			case '?':
				fprintf(stderr, "%s", error_opt);
				break;
		}
	}

	/* We check if everything is set */
	if( opt_flag != 3 ){
		fprintf(stderr, "You must set at least input and output\nUsage : \n%s\n", usage);
		return EXIT_FAILURE;
	}

	/* Gpio initialisation */
	ret=gpio_init(edge, GPIO_OUT,  gpio_out);
	if(ret == -1){
		fprintf(stderr, "Could not export gpio output %s\n", gpio_out);
		return EXIT_FAILURE;
	} else if (ret == -2 || ret == -3) {
		fprintf(stderr, "Could not set direction and/or edge for gpio output %s\n", gpio_out);
		gpio_clean(gpio_out);
	}

	ret=gpio_init(edge, GPIO_IN,  gpio_in);
	if(ret == -1){
		fprintf(stderr, "Could not export gpio input %s\n", gpio_in);
		return EXIT_FAILURE;
	} else if (ret == -2 || ret == -3) {
		fprintf(stderr, "Could not set direction and/or edge for gpio input %s\n", gpio_in);
		gpio_clean(gpio_in);
	}

	/* Handle interrupt */
	gpio_handle(gpio_in, gpio_out);

	/* Gpio clean */		
	gpio_clean(gpio_out);
	gpio_clean(gpio_in);

	return(EXIT_SUCCESS);
}
