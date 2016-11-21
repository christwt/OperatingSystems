/*
 * File: mixed.c
 * Author: William
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: 2016/5/11
 * Description:
 * 	This file will calculate pi over a given number of iterations (similar to cpu-
 * 	bound.c) and every 20th iteration will perform a r/w operation (similar to io-bound.c
 * 	in order to simulate a mixed IO/CPU process. Scheduling policy and 
 *      and differing vs same priorities may be specified. Pi calculating portion 
 * 	based on cpu-bound.c which was adapted from pi-sched.c by Andy Sayler and I/O
 * 	portion based on io-bound.c which was adapted from rw.c by Andy Sayler.
 */

/* Include Flags */
#define _GNU_SOURCE

/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>	
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sched.h>

/* Local defines*/
/* Pi calculating defines */
#define DEFAULT_ITERATIONS 100000
#define RADIUS (RAND_MAX / 2)
/* Read/Write defines */
#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100
/* Number of processes and same/different priorities defines */
#define LOW 10
#define MEDIUM 50
#define HIGH 150
#define SAME 0
#define DIFF 1

/* Pi calulating functions */
static inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}


static inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

/* main function to calculate pi and perform r/w operations */
int main(int argc, char* argv[]){
	 
    long i;
    long iterations = DEFAULT_ITERATIONS;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
    
	
    int rv, process_num, policy, diff, status;
    int inputFD;
    int outputFD;
    char inputFilename[MAXFILENAMELENGTH];
    char outputFilename[MAXFILENAMELENGTH];
    char outputFilenameBase[MAXFILENAMELENGTH];

    ssize_t transfersize = DEFAULT_TRANSFERSIZE;
    ssize_t blocksize = DEFAULT_BLOCKSIZE; 
    char* transferBuffer = NULL;
    ssize_t buffersize;

    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    int totalReads = 0;
    ssize_t bytesWritten = 0;
    ssize_t totalBytesWritten = 0;
    int totalWrites = 0;
    int inputFileResets = 0;
    
    struct sched_param param;
    pid_t pid, wpid;
    int children = 0;
    
    /* Set input and output filename and size */
    strncpy(inputFilename, DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH);
    strncpy(outputFilenameBase, DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH);

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
						  create nice value based on iterator. */
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
				
				// at every 1000th iteration, perform i/o operation.
				if ((i % 1000) == 0){
					/* Confirm blocksize is multiple of and less than transfersize*/
					if(blocksize > transfersize){
					fprintf(stderr, "blocksize can not exceed transfersize\n");
					exit(EXIT_FAILURE);
					}
					if(transfersize % blocksize){
					fprintf(stderr, "blocksize must be multiple of transfersize\n");
					exit(EXIT_FAILURE);
					}

					/* Allocate buffer space */
					buffersize = blocksize;
					if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
					perror("Failed to allocate transfer buffer");
					exit(EXIT_FAILURE);
					}
					
					/* Open Input File Descriptor in Read Only mode */
					if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
					perror("Failed to open input file");
					exit(EXIT_FAILURE);
					}

					/* Open Output File Descriptor in Write Only mode with standard permissions*/
					rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
						  outputFilenameBase, getpid());    
					if(rv > MAXFILENAMELENGTH){
					fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
						MAXFILENAMELENGTH);
					exit(EXIT_FAILURE);
					}
					else if(rv < 0){
					perror("Failed to generate output filename");
					exit(EXIT_FAILURE);
					}
					if((outputFD =
					open(outputFilename,
						 O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
						 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
					perror("Failed to open output file");
					exit(EXIT_FAILURE);
					}

					/* Print Status */
					fprintf(stdout, "Reading from %s and writing to %s\n",
						inputFilename, outputFilename);

					/* Read from input file and write to output file*/
					do{
					/* Read transfersize bytes from input file*/
					bytesRead = read(inputFD, transferBuffer, buffersize);
					if(bytesRead < 0){
						perror("Error reading input file");
						exit(EXIT_FAILURE);
					}
					else{
						totalBytesRead += bytesRead;
						totalReads++;
					}
					
					/* If all bytes were read, write to output file*/
					if(bytesRead == blocksize){
						bytesWritten = write(outputFD, transferBuffer, bytesRead);
						if(bytesWritten < 0){
						perror("Error writing output file");
						exit(EXIT_FAILURE);
						}
						else{
						totalBytesWritten += bytesWritten;
						totalWrites++;
						}
					}
					/* Otherwise assume we have reached the end of the input file and reset */
					else{
						if(lseek(inputFD, 0, SEEK_SET)){
						perror("Error resetting to beginning of file");
						exit(EXIT_FAILURE);
						}
						inputFileResets++;
					}
					
					}while(totalBytesWritten < transfersize);

					/* Output some possibly helpfull info to make it seem like we were doing stuff */
					fprintf(stdout, "Read:    %zd bytes in %d reads\n",
						totalBytesRead, totalReads);
					fprintf(stdout, "Written: %zd bytes in %d writes\n",
						totalBytesWritten, totalWrites);
					fprintf(stdout, "Read input file in %d pass%s\n",
						(inputFileResets + 1), (inputFileResets ? "es" : ""));
					fprintf(stdout, "Processed %zd bytes in blocks of %zd bytes\n",
						transfersize, blocksize);
					
					/* Free Buffer */
					free(transferBuffer);

					/* Close Output File Descriptor */
					if(close(outputFD)){
					perror("Failed to close output file");
					exit(EXIT_FAILURE);
					}

					/* Close Input File Descriptor */
					if(close(inputFD)){
					perror("Failed to close input file");
					exit(EXIT_FAILURE);
					}
						}
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
	
		
		
		
