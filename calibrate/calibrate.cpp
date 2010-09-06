
#include "calibrate.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "../parameters/parameters.h"



static volatile int stop_measurement;


static void *burn_cpu(void *dummy)
{
	volatile double d = 1.1;

	while (!stop_measurement) {
		d = pow(d, 1.0001);
	}
	return NULL;
}


static void cpu_calibration(int threads)
{	
	int i;
	pthread_t thr;

	printf( "Calibrating: CPU usage on %i threads\n", threads);

	stop_measurement = 0;
	for (i = 0; i < threads; i++)
		pthread_create(&thr, NULL, burn_cpu, NULL);

	one_measurement(30);
	stop_measurement = 1;
	sleep(1);
}



void calibrate(void)
{
	cpu_calibration(1);
	cpu_calibration(4);


        learn_parameters(400);
	printf("Parameters after calibration:\n");
	dump_parameter_bundle();
	save_parameters("saved_parameters.powertop");
        save_all_results("saved_results.powertop");
}