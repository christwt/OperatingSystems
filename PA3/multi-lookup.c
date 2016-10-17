/*
File: multi-lookup.c
Author: Will Christie
Project: CSCI 3753 Programming Assignment 3
Created Date: 2016-10-6
Description:
	Threaded solution: uses number of cores available to determine resolver threads. 
*/

#include "multi-lookup.h"

//condition variables
pthread_cond_t queueFull;		// signal/wait on queue full.
pthread_cond_t queueEmpty;		// signal/wait on queue empty. 
//mutex locks
pthread_mutex_t queueLock;		// to protect queue.
pthread_mutex_t requestorLock;		// to protect counter in readFiles section.
pthread_mutex_t resolverLock;		// to protext output file in dns section.
//queue
queue requestQ;				// instance of the queue.
int filesCompleted;			// protected counter to track the number of files added to the queue.
int numInputFiles;			// number of input files passed as arguments to executable.
char* outputFile;			// output file for dns lookup, protected.
int maxThreads;				// max number resolver threads, based on num available cores.

void* requestorPool(char* inputFiles) {
	// declare pointer to character array. necessary to prevent warning: "initialization makes pointer from integer without a cast [-Wint-conversion]"
	// lets us get to the char array. 
	char** inputFileNames = (char**) inputFiles;
	// declare requestor threads. 
	pthread_t requestorThreads[numInputFiles];
	int i;
	// create requestor threads until number of input files consumed. 
	for(i=0; i < numInputFiles; i++){
		char* fileName = inputFileNames[i];
		pthread_create(&requestorThreads[i], NULL, (void*) readFiles, (void*) fileName);
		// wait until requestor threads have finished. 
		pthread_join(requestorThreads[i], NULL);
	}
	for(i=0; i<numInputFiles; i++){
		// signal requestor threads that the queue is empty so can begin to work.
		pthread_cond_signal(&queueEmpty);
	}
	return NULL;
}

void* resolverPool(FILE* outputfp){
	// declare resolver threads. 
	pthread_t resolverThreads[numInputFiles];
	int i;
	// create resolver threads to perform dns lookup until maxThreads have been created.
	for(i=0; i<maxThreads; i++){
		pthread_create(&resolverThreads[i], NULL, dns, outputfp);
		// wait until resolver threads have finished. 
		pthread_join(resolverThreads[i], NULL);
	}
	return NULL;

}

void* readFiles(char* filename){
	// concatenate file path onto filename to match input example from handout.
	char filePath[SBUFSIZE] = "input/";
	strcat(filePath, filename);
	
	// open input file. 
	FILE* inputfp = fopen(filePath, "r");
	// Error Handling 3: Bogus Input File Path
	if(!inputfp){
		// print error to stderror
		perror("Error: unable to open input file.\n");
		// lock counter variable to increment. If not done results in infinite loop in dns().
		pthread_mutex_lock(&requestorLock);
		filesCompleted++;
		pthread_mutex_unlock(&requestorLock);
		return NULL;
	}
	char hostname[SBUFSIZE];
	// scan in file and get hostnames.
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		//check to see if queue is full. Need to protect queue access with mutex lock while doing this.
		pthread_mutex_lock(&queueLock);
		while(queue_is_full(&requestQ)){
			// if the queue is full then thread waits on condition variable queueFull. automatically unlocking queueLock and reaquiring it once signalled.
			pthread_cond_wait(&queueFull, &queueLock);
		}
		// otherwise, push hostname onto queue
		queue_push(&requestQ, strdup(hostname));
		// signal on condition queueEmpty to let consumer/resolver threads know that something is in the queue.
		pthread_cond_signal(&queueEmpty);
		// unlock queue access. 
		pthread_mutex_unlock(&queueLock);
	}
	// lock access to counter to increment number of files added to queue. 
	pthread_mutex_lock(&requestorLock);
	// increment counter in critical section.
	filesCompleted++;
	// unlock access to counter
	pthread_mutex_unlock(&requestorLock);
	// close the input file. 
	fclose(inputfp);
	return NULL;
}

void* dns(FILE* outputfp) {
	while(1){
		// lock queue mutex to access.
		pthread_mutex_lock(&queueLock);
		// check to see if queue is empty
		while(queue_is_empty(&requestQ)){
			// ensure that we lock files completed counter variable to access.
			pthread_mutex_lock(&requestorLock);
			int finished = 0;
			// determine if we are at the end of the queue. 
			if(filesCompleted == numInputFiles) finished = 1;
			// unlock counter variable
			pthread_mutex_unlock(&requestorLock);
			// we are finished so unlock queue. 
			if (finished){
				// unlock queue mutex
				pthread_mutex_unlock(&queueLock);
				return NULL;
			}
			// if the queue is empty but we still have files to consume that means that consumer threads must wait on condition variable emptyQueue.
			pthread_cond_wait(&queueEmpty, &queueLock);
		}
		// pop 1 hostname from the queue.
		char* hostname = (char*) queue_pop(&requestQ);
		// let producer processes know that there is space in the queue.
		pthread_cond_signal(&queueFull);
		// allocate IP address.
		char firstIp[MAX_IP_LENGTH];
		// if we are unable to perform dns lookup on bogus hostname.
		// Error Handling 1: bogus hostname. 
		if (dnslookup(hostname, firstIp, sizeof(firstIp))==UTIL_FAILURE){
			fprintf(stderr, "Error: DNS lookup failure on hostname: %s\n", hostname);
			strncpy(firstIp, "", sizeof(firstIp));
		}
		// lock the output file using resolver mutex lock.
		pthread_mutex_lock(&resolverLock);
		// add in the hostname with its IP address.
		fprintf(outputfp, "%s,%s\n", hostname, firstIp);
		// unlock resolver mutex to allow access to output file. 
		pthread_mutex_unlock(&resolverLock);
		// free space for next hostname. 
		free(hostname);
		// unlock queue access. 
		pthread_mutex_unlock(&queueLock);
	}
	return NULL;
}

int main(int argc, char* argv[]){
	filesCompleted = 0;
	numInputFiles = argc-2;
	outputFile = argv[argc-1];
	char* inputFiles[numInputFiles];
	
	// Extra Credit: determine number of cores and make sure we stick with at least MIN_RESOLVER_THREADS.
	// idea to use <unistd.h> sysconf from Stack Overflow post:
	// http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c 
	maxThreads = sysconf(_SC_NPROCESSORS_ONLN);
	if (maxThreads <= MIN_RESOLVER_THREADS) {
		maxThreads = MIN_RESOLVER_THREADS;
	}
	printf("Allocating resolver threads based on num cores available: %d threads\n", maxThreads);	

	// initialize queue as described in queue.h
	queue_init(&requestQ, QUEUEMAXSIZE);
	// initialize condition variables to signal/wait on queue full or empty. 
	pthread_cond_init(&queueFull, NULL);
	pthread_cond_init(&queueFull, NULL);
	// initialize mutex locks.
	pthread_mutex_init(&queueLock, NULL);
	pthread_mutex_init(&requestorLock, NULL);
	pthread_mutex_init(&resolverLock, NULL);	

	// Check arguments: as in lookup.c
	if (argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d", (argc-1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return(EXIT_FAILURE);
	}

	// Create array of input files. 
	int i;
	for(i=0; i<numInputFiles; i++){
		inputFiles[i] = argv[i+1];
	}
	
	//open output file.
	FILE* outputfp = fopen(outputFile, "w");
		// Error Handling 2: bogus output file. 
		if(!outputfp){
		// print to std error and exit.
		perror("Error: unable to open output file");
		exit(EXIT_FAILURE);
	}

	// declare requestor ID thread, used to initialize requestor thread pool.
	pthread_t requestorID;

	// requestor = producer, startup for requestor pool.
	int producer = pthread_create(&requestorID, NULL, (void*) requestorPool, inputFiles);
	if (producer != 0){
		errno = producer;
		perror("Error: pthread_create requestor");
		exit(EXIT_FAILURE);
	}

	// declare resolver ID thread, used to initialize resolver thread pool.
	pthread_t resolverID;
	
	// resolver = consumer, startup for resolver pool.
	// pthread_create(pthread_t *threadName, const pthread_attr * attr, void*(*start_routine), void* arg)
	int consumer = pthread_create(&resolverID, NULL, (void*) resolverPool, outputfp);
	if (consumer != 0){
		errno = consumer;
		perror("Error: pthread_create resolver");
		exit(EXIT_FAILURE);
	}

	// wait for threads to finish using pthread_join. as in pthread_hello.c
	// pthread_join suspends execution of calling thread ie(resolverID/requestorID) until execution is complete.
	// until threads in resolver pool have completed their tasks. 
	pthread_join(requestorID, NULL);
	pthread_join(resolverID, NULL);

	// close output file. 
	fclose(outputfp);
	// free queue memory
	queue_cleanup(&requestQ);
	// destroy mutex locks
	pthread_mutex_destroy(&resolverLock);
	pthread_mutex_destroy(&queueLock);
	pthread_mutex_destroy(&requestorLock);
	// destroy condition variables. 
	pthread_cond_destroy(&queueEmpty);
	pthread_cond_destroy(&queueFull);
	// last thing a program should do, from pthread-hello.c, should be unecessary due to pthread_join statements. 
	pthread_exit(NULL);

	return EXIT_SUCCESS;

}
