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

#define NOINIT 0
#define INIT 1

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

/* Input and output file descriptors, values */
char gpio_in[8], gpio_out[8];
int gpio_out_fd, gpio_in_fd;
fd_set gpio_in_fds;
char output_value;

int initialize = INIT;

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
	if(open_and_write("/sys/class/gpio/export", gpio_no, sizeof(char)*strnlen(gpio_no, 8)) < 0){
		fprintf(stderr, "Could not export gpio %s\n", gpio_no);
		return -1;
	}

	/* Set direction */
	if(snprintf(buf, 128, "/sys/class/gpio/gpio%s/direction", gpio_no) < 0){
		fprintf(stderr, "Could not create direction string for gpio %s\n", gpio_no);
		return -2;
	}
	if(open_and_write(buf, direction, sizeof(char)*strnlen(direction, 3)) < 0){
		fprintf(stderr, "Could not set direction for gpio %s\n", gpio_no);
		return -2;
	}

	/* Set edge */
	if(snprintf(buf, 128, "/sys/class/gpio/gpio%s/edge", gpio_no) < 0){
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
 * return :		EXIT_SUCCESS
 * 				EXIT_FAILURE
 */
int gpio_clean(const char* gpio_no)
{
	/* Unexport gpio_no */
	if(open_and_write("/sys/class/gpio/unexport", gpio_no, sizeof(char)*strnlen(gpio_no, 8)) < 0){
		fprintf(stderr, "Could not unexport gpio %s\n", gpio_no);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 * Reverses the value of the output gpio when an interrupt occurs on the input gpio
 *
 * return	:	EXIT_SUCCESS on success
 * 				EXIT_FAILURE on failure
 */
int gpio_handler()
{
	char gpio_input_file[128];
	char gpio_output_file[128];
	char input_value[2];

	
	/* Open the input file */
	if(snprintf(gpio_input_file, 128, "/sys/class/gpio/gpio%s/value", gpio_in) < 0){
		fprintf(stderr, "An error occured while opening value file for input gpio %s\n", gpio_in);
		return EXIT_FAILURE;
	}
	if((gpio_in_fd=open(gpio_input_file, O_RDONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_input_file);
		return EXIT_FAILURE;
	}

	/* Open the output file */
	if(snprintf(gpio_output_file, 128, "/sys/class/gpio/gpio%s/value", gpio_out) < 0){
		fprintf(stderr, "An error occured while opening value file for output gpio %s\n", gpio_out);
		return EXIT_FAILURE;
	}
	if((gpio_out_fd=open(gpio_output_file, O_WRONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_output_file);
		return EXIT_FAILURE;
	}

	/* Set output low */
	lseek(gpio_out_fd, 0, SEEK_SET);
	output_value = GPIO_LOW;
	if(write(gpio_out_fd, &output_value, 1)!=1){
		fprintf(stderr, "Could not write the value %c to %s\n", output_value, gpio_output_file);
	}

	/* The first select will work immediately */
	/* Prepare event table */
	FD_ZERO(&gpio_in_fds);
	FD_SET(gpio_in_fd, &gpio_in_fds);

	/* Sleep untill event, no timeout */
	if(select(gpio_in_fd+1, NULL, NULL, &gpio_in_fds, NULL) < 0){
		fprintf(stderr, "Select failed\n");
		return EXIT_FAILURE;
	}
	/* Read the input value (otherwise select will not wait) */
	lseek(gpio_in_fd, 0, SEEK_SET);
	if(read(gpio_in_fd, &input_value, 2) != 2){
		fprintf(stderr, "Could not read the value from %s\n", gpio_in);
		return EXIT_FAILURE;
	}

	/* Main loop */
	while(1) {
		/* Prepare event table */
		FD_ZERO(&gpio_in_fds);
		FD_SET(gpio_in_fd, &gpio_in_fds);

		/* Sleep untill event, no timeout */
		if(select(gpio_in_fd+1, NULL, NULL, &gpio_in_fds, NULL) < 0){
			fprintf(stderr, "Select failed\n");
			break;
		}
		
		/* Reverse the value */
		lseek(gpio_out_fd, 0, SEEK_SET);
		output_value = (output_value == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW;
		if(write(gpio_out_fd, &output_value, 1) != 1){
			fprintf(stderr, "Could not write the value %c to %s\n",output_value, gpio_output_file);
			break;
		}

		/* Read the input value (otherwise select will not wait) */
		lseek(gpio_in_fd, 0, SEEK_SET);
		if(read(gpio_in_fd, &input_value, 2) != 2){
			fprintf(stderr, "Could not read the value from %s\n", gpio_in);
			break;
		}
	}

	/* We should never get here but still... */
	close(gpio_in_fd);
	close(gpio_out_fd);
	return EXIT_SUCCESS;
}

/**
 * Trigger, called periodically with timer
 * This is where all the handling happens
 *
 */
void gpio_trigger_sighandler(int unused)
{
	struct timeval start_time, stop_time;
	long long int latency;
	char input_value[2];

	fprintf(stderr, "Triggered\n");

	/* Prepare event table */
	FD_ZERO(&gpio_in_fds);
	FD_SET(gpio_in_fd, &gpio_in_fds);

	/* Set output high */
	lseek(gpio_out_fd, 0, SEEK_SET);
	output_value = GPIO_HIGH;
	if(write(gpio_out_fd, &output_value, 1) != 1){
		fprintf(stderr, "Could not write the value %c to %s\n", output_value, gpio_out);
		return;
	}

	/* Measure time */
	gettimeofday(&start_time, NULL);

	/* Sleep untill event, no timeout */
	if(select(gpio_in_fd+1, NULL, NULL, &gpio_in_fds, NULL) < 0){
		fprintf(stderr, "Select failed\n");
		return;
	}

	/* Measure time */
	gettimeofday(&stop_time, NULL);

	/* Reverse the output value */
	lseek(gpio_out_fd, 0, SEEK_SET);
	output_value = (output_value == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW;
	if(write(gpio_out_fd, &output_value, 1) != 1){
		fprintf(stderr, "Could not write the value %c to %s\n", output_value, gpio_out);
		return;
	}

	/* Read the input value (otherwise select will not wait) */
	lseek(gpio_in_fd, 0, SEEK_SET);
	if(read(gpio_in_fd, &input_value, 2) != 2){
		fprintf(stderr, "Could not read the value for input gpio %s\n", gpio_in);
		return;
	}

	/* Compute and print */
	/* FIX OVERFLOW problem */
	latency = stop_time.tv_sec - start_time.tv_sec;
	latency *= 1000000;
	latency = stop_time.tv_usec - start_time.tv_usec;

	fprintf(stdout, "%06lld\n", latency);
}

/**
 * Periodically sets output gpio high and measure latency of new stateon input gpio
 *
 * trigger_period	:	period
 * return	:	EXIT_SUCCESS
 * 				EXIT_FAILURE
 */
int gpio_trigger(unsigned int trigger_period)
{
	char gpio_output_file[128];
	char gpio_input_file[128];
	char input_value[2];

	timer_t timer;
	struct sigevent event;
	struct itimerspec timerspec;

	/* Open the input file */
	if(snprintf(gpio_input_file, 128, "/sys/class/gpio/gpio%s/value", gpio_in) < 0){
		fprintf(stderr, "An error occured while opening value file for input gpio %s\n", gpio_in);
		return EXIT_FAILURE;
	}
	if((gpio_in_fd=open(gpio_input_file, O_RDONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_input_file);
		return EXIT_FAILURE;
	}

	/* Open the output file */
	if(snprintf(gpio_output_file, 128, "/sys/class/gpio/gpio%s/value", gpio_out) < 0){
		fprintf(stderr, "An error occured while opening value file for input gpio %s\n", gpio_out);
		return EXIT_FAILURE;
	}
	if((gpio_out_fd=open(gpio_output_file, O_WRONLY)) < 0){
		fprintf(stderr, "Could not open %s\n", gpio_output_file);
		return EXIT_FAILURE;
	}

	/* Set output low */
	lseek(gpio_out_fd, 0, SEEK_SET);
	output_value = GPIO_LOW;
	if(write(gpio_out_fd, &output_value, 1)!=1){
		fprintf(stderr, "Could not write the value %c to %s\n", output_value, gpio_output_file);
	}

	/* The first select will wor immediately */
	/* Prepare event table */
	FD_ZERO(&gpio_in_fds);
	FD_SET(gpio_in_fd, &gpio_in_fds);

	/* Sleep untill event, no timeout */
	if(select(gpio_in_fd+1, NULL, NULL, &gpio_in_fds, NULL) < 0){
		fprintf(stderr, "Select failed\n");
		return EXIT_FAILURE;
	}
	/* Read the input value (otherwise select will not wait) */
	lseek(gpio_in_fd, 0, SEEK_SET);
	if(read(gpio_in_fd, &input_value, 2) != 2){
		fprintf(stderr, "Could not read the value from %s\n", gpio_in);
		return EXIT_FAILURE;
	}

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

/**
 * Clean exit after the end of duration time 
 *
 */
void alarm_sighandler(int unused)
{
	/* Close gpio files */
	close(gpio_in_fd);
	close(gpio_out_fd);

	/* Unexport gpios */
	if(initialize == INIT){
		gpio_clean(gpio_in);
		gpio_clean(gpio_out);
	}

	/* We are done */
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	/* Variables */
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
		{"noinit",		0,		NULL,		'3'},
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
			case '3':
				initialize = NOINIT;
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

	/* Clean exit on Ctrl-C */
	signal(SIGINT, alarm_sighandler);

	/* Set alarm if duration is set */
	if(duration != 0){
		signal(SIGALRM, alarm_sighandler);
		alarm(duration);
	}

	/* Gpio initialisation */
	if(initialize == INIT){
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
	}

	/* Handle/Trigger interrupt */
	if(type==TYPE_HANDLER){
		if(gpio_handler() < 0){
			fprintf(stderr, "Could not start handler loop\n");
			gpio_clean(gpio_out);
			gpio_clean(gpio_in);
			return EXIT_FAILURE;
		}
	} else {
		if(gpio_trigger(trigger_period) == EXIT_FAILURE){
			fprintf(stderr, "Could not start trigger loop\n");
			gpio_clean(gpio_out);
			gpio_clean(gpio_in);
			return EXIT_FAILURE;
		}
	}

	/* Gpio clean */		
	if( initialize == INIT){
		gpio_clean(gpio_out);
		gpio_clean(gpio_in);
	}

	return EXIT_SUCCESS;
}
