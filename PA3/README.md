###William Christie

SID: 810915676

Email: william.christie@colorado.edu

Programming Assignment 3: Use pthreads to code a DNS name resolution engine.

### Submission Contents:
a. multi-lookup.c
    
    Application which takes as input a set of name files, containing 1 hostname per line. 
    Each name file is serviced by a single requestor thread from the requester pool and serviced 
    by the maximum number of resolver threads to output each hostname with its IP address. 

b. multi-lookup.h
    
    Header file for multi-lookup.h

c. queue.c
    
    Provided non-thread-safe queue. Requester threads push pointers to C-string hostnames to 
    queue and resolver threads pop from queue. FIFO principle. 

d. queue.h
  
    Header file for queue.c

e. util.c
   
    Provided utility function to perform dns lookup based on given hostname and return the resolved IP address. 

f. util.h
    
    Header file for util.h

g. Makefile
  
    GNU makefile to build all code listed. 

h. input/
  
    Folder containing name files of hostnames. 

## To Run:
1. Download all code and place in directory. 
2. Run make to build all files. 
3. Run application using the following command:

        ./multi-lookup names1.txt ... namesN.txt results.txt
 
4. To check for memory leaks run:

        sudo apt-get install valgrind 
        valgrind ./multilookup names1.txt...namesN.txt results.txt
        
4. Run make clean to remove files. 

