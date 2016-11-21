/*
 * File: cpu-bound.c
 * Author: Andy Sayler (pi-sched.c)
 * Revised: Dhivakant Mishra (pi-sched.c)
 * Adapted: William Christie
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: 2012/03/07
 * Modify Date: 2012/03/09
 * Modify Date: 2016/31/10
 * Adapted Date: 2016/5/11
 * Description:
 *      This file contains a simple program for statistically
 *      calculating pi using a specified number of processes, 
 *	a given scheduling policy, and optional scheduling 
 *	niceness/priority level. Adapted from pi-sched.c by Andy Sayler
 */

/* Local Includes*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>		
#include <sys/resource.h>   	// for setpriority()
#include <sys/time.h>		// for setpriority()
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

/* Local defines*/
#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)
/* Number of processes and same/different priorities defines */
#define LOW 10
#define MEDIUM 50
#define HIGH 150
#define SAME 0
#define DIFF 1

static inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}


static inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

int main(int argc, char* argv[]){

	long i;
	long iterations = DEFAULT_ITERATIONS;
	struct sched_param param;
	int policy, process_num, diff, status;
	double x, y;
	double inCircle = 0.0;
	double inSquare = 0.0;
	double pCircle = 0.0;
	double piCalc = 0.0;
	pid_t pid, wpid;
	int children = 0;

	/* set default policy if not suppilied */
	if(argc < 2){
		policy = SCHED_OTHER;
	}

	/* set default number of processes if not supplied */
	if(argc < 3){
		process_num = LOW;
	}
	
	/* set default priority for processes to be same or different if not supplied */
	if (argc < 4){
		diff = SAME;
	}

	/* set policy if supplied */
	if(argc > 1){
		if(!strcmp(argv[1], "SCHED_OTHER")){
			policy = SCHED_OTHER;
		}
		else if(!strcmp(argv[1], "SCHED_FIFO")){
			policy = SCHED_FIFO;
		}
		else if(!strcmp(argv[1], "SCHED_RR")){
			policy = SCHED_RR;
		}
		else{
			fprintf(stderr, "Unhandeled scheduling policy\n");
			exit(EXIT_FAILURE);
		}
	}
	
	/* set number of simultaneous processes */
	if(argc > 2){
		if(!strcmp(argv[2], "LOW")){
			process_num = LOW;
		}
		else if(!strcmp(argv[2], "MEDIUM")){
			process_num = MEDIUM;
		}
		else if(!strcmp(argv[2], "HIGH")){
			process_num = HIGH;
		}
		else{
			fprintf(stderr, "Unhandeled number of processes\n");
			exit(EXIT_FAILURE);
		}	
	}
	
	/* set different or same priority/niceness level */
	if(argc > 3){
		if(!strcmp(argv[3], "SAME")){
			diff = SAME;
		}
		else if(!strcmp(argv[3], "DIFF")){
			diff = DIFF;
		}
	}
	
	/* set process to max priority for given scheduler */
	param.sched_priority = sched_get_priority_max(policy);
	fprintf(stdout, "Max priority for given policy: %d\n", sched_get_priority_max(policy));
	fprintf(stdout, "Min priority for given policy: %d\n", sched_get_priority_min(policy));

	/* set different scheduling policy*/
	fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
	fprintf(stdout, "Setting Scheduling Policy: %d\n", policy);
	if(sched_setscheduler(0, policy, &param)){
		perror("Error setting scheduler policy");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

	/* fork the number of child  processes specified to run pi code */
	fprintf(stdout, "Number of processes to be forked: %d\n", process_num);
	for(i = 0; i < process_num; i++){
		if((pid = fork()) == -1){
			fprintf(stderr, "Error forking child process");
			exit(EXIT_FAILURE);
		}
		/* run pi calculation code for child processes */
		if(pid == 0){
			/* reset priority or niceness level if indicated by diff */
			if(diff){
					/*SCHED_OTHER, change niceness range [-20,19]*/
					if(policy == SCHED_OTHER){
						/*use setpriority() syscall to increment priority.
						  same as nice() but can set to a specific priority
					       	  instead of incrementing with nice().
						  create nice value based on iterator.*/ 
						int nice = i % 15; 
						setpriority(PRIO_PROCESS, 0, nice); 
						//check new process priority and print.
						int newNice = getpriority(PRIO_PROCESS, 0);
						fprintf(stdout, "New child process niceness: %d\n", newNice);
					}
					/*SCHED_FIFO and SCHED_RR, change priority range [0,99]*/
					else if(policy == SCHED_FIFO){
						/*generate int between 0,99
						  sched_setparam() to set the new priority for the process.*/
						int p = i % 10 + 5;
						param.sched_priority = p;
						// set new priority based on on iterator calculation using sched_set_scheduler().
						sched_setscheduler(0, policy, &param);
						fprintf(stdout, "New Scheduling priority: %d\n", p);
					}
					else if(policy == SCHED_RR){
						int p = i % 10 + 5;
						param.sched_priority = p;
						// set new priority based on on iterator calculation using sched_set_scheduler().
						sched_setscheduler(0, policy, &param);
						fprintf(stdout, "New Scheduling priority: %d\n", p);
						struct timespec ts;
						// utilize sched_rr_get_interval() to obtain timeslice info.
						sched_rr_get_interval(0, &ts);
						fprintf(stdout, "Timeslice: %lu.%lu seconds\n", ts.tv_sec, ts.tv_nsec);
					}
			}
			
			/* calculate pi using statistical method across all iterations */
			for(i = 0; i < iterations; i++){
				x = (random() % (RADIUS * 2)) - RADIUS;
				y = (random() % (RADIUS * 2)) - RADIUS;
				if(zeroDist(x,y) < RADIUS){
					inCircle++;
				}
				inSquare++;
			}
			
			pCircle = inCircle/inSquare;
			piCalc = pCircle * 4.0;
			
			fprintf(stdout, "pi, %f\n", piCalc);
			
			exit(0);
		}
	}
	
	/* parent process will wait for all child processes to finish and reaps them */
	while((wpid = wait(&status)) > 0){
		if(WIFEXITED(status)){
			fprintf(stdout, "Child process: %d exited normally\n", wpid);
			children++;
		}
	}
	fprintf(stdout, "Parent reaped: %d processes\n", children);
	return 0;
}

