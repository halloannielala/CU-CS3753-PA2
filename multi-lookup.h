/*
 * 
File: multi-lookup.c
Author: Anne Gatchell
Project: CSCI 3753 Programming Assignment 2
Create Date: 2013/02/25
Modify Date: 2013/02/27

Adapted from:
File: lookup.c
 * Author: Andy Sayler


Description: This is the header file for a producer consumer
DNS Resolution Engine.
 */


#ifndef MULTILOOKUP_H
#define MULTILOOKUP_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath1> (other input filepaths) <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define NUM_THREADS 10
#define QUEUE_SIZE 1
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

/* Function for Producer threads
to run. One thread is run per input file.
So, each thread opens the file is is
given in its argument, and it reads input 
names from it and adds them to the queue as
the queue has room available. When the 
file has been read entirely, the thread 
finishes and exits.
*/
void* Producer(void* fn);

/*
Function for the consumer threads 
in the DNS Resolution Engine. Threads
are used to pop a domain name from the 
queue, look up its IP address, and then 
print both to an output file.
Threads exit when they discover that
all the producer threads have finished
(a boolean variable) and the queue is 
empty.
In order to make sure that all threads 
exit. The first exiting thread increments
the full count semaphore many times in order
to free any threads from a fruitless blocking
situation. Those threads then realize the queue
is empty and exit.
*/
void* Consumer(void* threadid);

#endif