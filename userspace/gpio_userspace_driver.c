#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
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
 * Unexports gpio
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

/**
 * Reverses the value of the output gpio when an interrupt occurs on the input gpio
 *
 * return	:	EXIT_SUCCESS on sucess
 * 				EXIT_FAILURE on failure
 */
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

/**
 * Trigger
 *
 */
void gpio_trigger_sighandler(int unused)
{
	fprintf(stderr, "Triggered\n");
}

/**
 * Periodically sets output gpio high and measure latency of new stateon input gpio
 *
 * gpio_in	:	input gpio
 * gpio_out	:	output_gpio
 * trigger_period	:	period
 * return	:	EXIT_SUCCESS
 * 				EXIT_FAILURE
 */
int gpio_trigger(const char* gpio_in, const char* gpio_out, unsigned int trigger_period)
{
	timer_t timer;
	struct sigevent event;
	struct itimerspec timerspec;

	/* Open input gpio */

	/* Open output gpio */

	/* Set output to low */

	/* Set timer */
	signal(SIGRTMIN+1, gpio_trigger_sighandler);

	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGRTMIN+1;
	
	timerspec.it_interval.tv_sec = 0;
	timerspec.it_interval.tv_nsec = trigger_period;
	timerspec.it_value = timerspec.it_interval;
	if(timer_create(CLOCK_REALTIME, &event, &timer) != 0){
		fprintf(stderr, "Could not create timer\n");
		return EXIT_FAILURE;
	}

	if(timer_settime(timer, 0, &timerspec, NULL) != 0 ){
		fprintf(stderr, "Could not start timer\n");
		return EXIT_FAILURE;
	}

	/* Main loop */
		while(1)
			pause();

	/* We should never get here */
	return EXIT_SUCCESS;
}

/* Clean exit after the end of duration time */
void alarm_sighandler(int unused)
{
	fprintf(stderr, "SIGALRM\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	/* Variables */
	char gpio_in[8], gpio_out[8];
	unsigned int trigger_period = 250000000;
	unsigned int duration = 0;

	int ret;
	int opt_flag = 0; /* will test if all the values are set */
	int type = TYPE_HANDLER; /* defines if the tigger or handler mode should be set */

	/* Command line arguments */
	char* usage = "\t-ioph --triger --handler";
	int option;
	int longindex;
	char* optstring = "i:o:p:d:h";
	char* error_opt = "Wrong option";
	struct option longopts[] = {
		{"in",			1,		NULL,		'i'},
		{"out",			1,		NULL,		'o'},
		{"period",		1,		NULL,		'p'},
		{"duration",	1,		NULL,		'd'},
		{"help",			0,		NULL,		'h'},
		{"handler",		0,		NULL,		'1'},
		{"trigger",		0,		NULL,		'2'},
		{NULL,			0,		NULL,		0},
		};

	while( (option=getopt_long(argc, argv, optstring, longopts, &longindex)) != -1){
		switch(option){
			case 'i':
				if(sscanf(optarg, "%s", gpio_in) != 1){
					fprintf(stderr, "Could not set gpio input\n");
					return EXIT_FAILURE;
				}
				opt_flag= opt_flag | 1;
				break;
			case 'o':
				if(sscanf(optarg, "%s", gpio_out) != 1){
					fprintf(stderr, "Could not set gpio output\n");
					return EXIT_FAILURE;
				}
				opt_flag= (opt_flag & 1) | 2;
				break;
			case '1':
				type=TYPE_HANDLER;
				break;
			case '2':
				type=TYPE_TRIGGER;
				break;
			case 'p':
				if(sscanf(optarg, "%u", &trigger_period) != 1){
					fprintf(stderr, "Could not set trigger period\n");
					return EXIT_FAILURE;
				}
				break;
			case 'd':
				if(sscanf(optarg, "%u", &duration) != 1){
					fprintf(stderr, "Could not set duration\n");
					return EXIT_FAILURE;
				}
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

	/* Set alarm if duration is set */
	if(duration != 0){
		signal(SIGALRM, alarm_sighandler);
		alarm(duration);
	}

	/* Gpio initialisation */
	ret=gpio_init(GPIO_NONE_EDGE, GPIO_OUT,  gpio_out);
	if(ret == -1){
		fprintf(stderr, "Could not export gpio output %s\n", gpio_out);
		return EXIT_FAILURE;
	} else if (ret == -2 || ret == -3) {
		fprintf(stderr, "Could not set direction and/or edge for gpio output %s\n", gpio_out);
		gpio_clean(gpio_out);
		return EXIT_FAILURE;
	}

	ret=gpio_init(GPIO_BOTH_EDGE, GPIO_IN,  gpio_in);
	if(ret == -1){
		fprintf(stderr, "Could not export gpio input %s\n", gpio_in);
		gpio_clean(gpio_out);
		return EXIT_FAILURE;
	} else if (ret == -2 || ret == -3) {
		fprintf(stderr, "Could not set direction and/or edge for gpio input %s\n", gpio_in);
		gpio_clean(gpio_in);
		gpio_clean(gpio_out);
		return EXIT_FAILURE;
	}

	/* Handle/Trigger interrupt */
	if(type==TYPE_HANDLER){
		if(gpio_handle(gpio_in, gpio_out) < 0){
			fprintf(stderr, "Could not start handler loop\n");
			gpio_clean(gpio_out);
			gpio_clean(gpio_in);
			return EXIT_FAILURE;
		}
	} else {
		if(gpio_trigger(gpio_in, gpio_out, trigger_period) == EXIT_FAILURE){
			fprintf(stderr, "Could not start trigger loop\n");
			gpio_clean(gpio_out);
			gpio_clean(gpio_in);
			return EXIT_FAILURE;
		}
	}

	/* Gpio clean */		
	gpio_clean(gpio_out);
	gpio_clean(gpio_in);

	return EXIT_SUCCESS;
}
