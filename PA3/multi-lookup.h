#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

// libraries as in lookup.c and pthread_hello_world.c
#include <pthread.h>		// pthreads!
#include <stdlib.h>		
#include <stdio.h>		// for file I/O, stderr
#include <errno.h>		// for errno
#include <unistd.h>		// for sysconf(_SC_NPROCESSORS_ONLN)

// necessary functions to include:
#include "util.h"		// for DNS lookup
#include "queue.h"		// queue for hostnames.

// global variables as in lookup.c
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
// additional assignment specific definitions.
// did not utilize: MAX_THREADS, MAX_INPUT_FILES, chose not to cap those definitions.
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define MIN_RESOLVER_THREADS 2

// function to push hostnames into the queue.
void* readFiles(char* filename);

// create "Producer" thread pool
void* requestorPool(char* inputFiles);

// function to perform DNS lookup.
void* dns();

// create "Consumer" thread pool
void* resolverPool();

// main
int main(int argc, char* argv[]);

#endif
