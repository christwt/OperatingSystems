/*
 * File: io-bound.c
 * Author: Andy Sayler
 * Revised: Shivakant Mishra
 * Adapted: William Christie
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: 2012/30/10
 * Modify Date: 2016/30/10
 * Adapted Date: 2016/5/11
 * Description: A small i/o bound program to copy N bytes from an input
 *              file to an output file. May read the input file multiple
 *              times if N is larger than the size of the input file.
 * 				Adapted from rw.c by Andy Sayler.
 */


/* Include Flags */
#define _GNU_SOURCE

/* System Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>		// for setpriority()
#include <sys/time.h>			// for setpriority()
#include <sys/stat.h>
#include <sys/wait.h>
#include <sched.h>

/* Local Defines */
#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100
/* Number of processes and same/different priorities defines */
#define LOW 10
#define MED 50
#define HIGH 150
#define SAME 0
#define DIFF 1

int main(int argc, char* argv[]){

    int rv, process_num, policy, diff, status, i;
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

    /* Set default policy if not supplied */
    if(argc < 2){
        policy = SCHED_OTHER;
    }

    /* Set default number of processes */
    if(argc < 3){
        process_num = LOW;
    }
    
    	/* set default priority for processes to be same or different if not supplied */
	if (argc < 4){
		diff = SAME;
	}
    
    /* Set policy if supplied */
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

    /* Set # Of Processes */
    if(argc > 2){
        if(!strcmp(argv[2], "LOW")){
            process_num = LOW;
        }
        else if(!strcmp(argv[2], "MEDIUM")){
            process_num = MED;
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
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

	/* Fork number of processes */
    printf("Number of processes to be forked %d \n", process_num);
    for(i = 0; i < process_num; i++){
        if((pid = fork())==-1){
            fprintf(stderr, "Error Forking Child Process");
            exit(EXIT_FAILURE);
        } 
        /* Run read/write code from child processes */
        if(pid == 0){
			if(diff){
				/*SCHED_OTHER, change niceness range [-20,19]*/
				if(policy == SCHED_OTHER){
					//use setpriority() syscall to increment priority.						
					//same as nice() but can set to a specific priority
					//instead of incrementing. 
					//get nice value based on iterator value i.
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

		    exit(0);
		}
	}
	/* Parent process waits for all child processes to terminate and reaps them. */
    while((wpid = wait(&status)) > 0){
        if(WIFEXITED(status)){
        printf("Child process %d exited normally\n", wpid);
        children++;
        }
    }
    printf("Parent reaped: %d processes\n", children);
    return EXIT_SUCCESS;  
}
