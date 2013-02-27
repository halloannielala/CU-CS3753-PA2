/*
 * File: lookup.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2012/02/01
 * Modify Date: 2012/02/01
 * Description:
 * 	This file contains the reference non-threaded
 *      solution to this assignment.
 *  
 */

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
#define QUEUE_SIZE 5

FILE* outputfp = NULL;
queue q;
int doneWritingToQueue = 0;
int processIsDone = 0;
sem_t sem_full;
sem_t sem_empty;
sem_t sem_m;
sem_t sem_results;

/* Function for Each Thread to Run */
void* Producer(void* fn)
{
    /* Setup Local Vars and Handle void* */
    char* filename = fn;
    //long t;
    long numprint = 3;
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    FILE* inputfp = NULL;
    int valp;

	/* Open Input File */
	inputfp = fopen(filename, "r");
	if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", filename);
	    perror(errorstr);
	}

    //sem_getvalue(&sem_empty, &valp);
    //printf("VALUE of EMPTY Semaphore before while loop %d\n", valp);

	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		/*Decrement full semaphore and acquire queue lock*/
        sem_getvalue(&sem_empty, &valp);
        printf("Thread %s INPUTFS empty %d",filename, valp);
        sem_getvalue(&sem_full, &valp);
        printf(" full %d\n",valp);

        sem_wait(&sem_empty);
        sem_wait(&sem_m);
        // sem_getvalue(&sem_empty, &valp);
        // printf("VALUE of EMPTY Semaphore after entering lock %d\n",valp);
		/*Add name to queue*/
        if(queue_push(&q, hostname) == QUEUE_FAILURE){
            fprintf(stderr,
                "error: queue_push failed!\n"
                "Thread %s, Name: %s\n",
                filename, hostname);
        }
        printf("Thread: %s pushed %s to queue\n", filename, hostname);
	    /*Unlock queue*/
        sem_post(&sem_m);
        sem_post(&sem_full);
	}
    
	/* Close Input File */
	fclose(inputfp);

    /* Exit, Returning NULL*/
    return NULL;
}

void* Consumer(void* threadid){
    //char hostname[SBUFSIZE];
    char* hostname;
    long* tid = threadid;
    //char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    int valp;

    while(!doneWritingToQueue && !processIsDone){
        sem_getvalue(&sem_empty, &valp);
        printf("Thread %ld is empty %d",*tid, valp);
        sem_getvalue(&sem_full, &valp);
        printf(" full %d\n",valp);

    	/*Decrement empty stderremaphore and acquire queue lock*/
        sem_wait(&sem_full);
        sem_wait(&sem_m);

    	/*Get a name*/
        if((hostname = queue_pop(&q)) == NULL){
            fprintf(stderr,
                "error: queue_pop failed!\n"
                "Threadid: %p\n",
                threadid);
        }
        printf("Thread: %ld, Name: %s from queue\n", *tid, hostname);
        /*Check if the queue is empty and the producers are done
            and set flag so.*/
        if(queue_is_empty(&q) && doneWritingToQueue){
            processIsDone = 1;
        }
    	/*Unlock queue*/
        sem_post(&sem_m);
        sem_post(&sem_empty);

    	/* Lookup hostname and get IP string */
        if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
          				 == UTIL_FAILURE){
    		fprintf(stderr, "dnslookup error: %s\n", hostname);
    		strncpy(firstipstr, "", sizeof(firstipstr));  
        }

        /*Lock file*/
        sem_wait(&sem_results);
        /* Write to Output File */
        printf("Thread: %ld to file\n", *tid);
    	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
    	/*Unlock file*/
        sem_post(&sem_results);
    }
	return NULL;
}


int main(int argc, char* argv[]){

    /* Local Vars */
    
    // char hostname[SBUFSIZE];
    // char errorstr[SBUFSIZE];
    // char firstipstr[INET6_ADDRSTRLEN];
    int i;
    int numinputfiles;
    pthread_t consumer_threads[NUM_THREADS];
    
    /* Setup Local Vars */
    int rc;
    long t;
    long cpyt[NUM_THREADS];

    /*Initialize semaphores*/
    if(sem_init(&sem_full, 0, 0) == -1){
        fprintf(stderr, "Error creating sem_init\n");
    }
     if(sem_init(&sem_empty, 0, QUEUE_SIZE) == -1){
        fprintf(stderr, "Error creating sem_init\n");
    }
     if(sem_init(&sem_m, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_init\n");
    }
     if(sem_init(&sem_results, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_init\n");
    }

    /* Check Arguments */
    if(argc < MINARGS){
    	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
    	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
    	return EXIT_FAILURE;
    }

    /*Calculate number of input files*/
    numinputfiles = argc - 2;

    /*Initialize the array of producer threads to be the number of 
    input files */
    pthread_t producer_threads[numinputfiles];

    /* Initialize Queue */
    if(queue_init(&q, QUEUE_SIZE) == QUEUE_FAILURE){
	fprintf(stderr,
		"error: queue_init failed!\n");
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
    	perror("Error Opening Output File");
    	return EXIT_FAILURE;
    }

    

    int valp;
    sem_getvalue(&sem_empty, &valp);
    printf("VALUE of EMPTY Semaphore main %d\n", valp);

    /* Loop Through Input Files and Create Producer Threads */
    for(t=0; t<numinputfiles; t++){
		printf("In main: creating producer thread %ld, %s\n", t, argv[t+1]);
		rc = pthread_create(&(producer_threads[t]), NULL, Producer, argv[t+1]);
		if (rc){
		    printf("ERROR; return code from pthread_create() is %d\n", rc);
		    exit(EXIT_FAILURE);
		}
	}	
    t = 0;
	 /* Spawn NUM_THREADS Consumer threads */
     for(t=0;t<NUM_THREADS;t++){
	 	printf("In main: creating consumer thread %ld\n", t);
	 	cpyt[t] = t;
	 	rc = pthread_create(&(consumer_threads[t]), NULL, Consumer, &(cpyt[t]));
	 	if (rc){
	 	    printf("ERROR; return code from pthread_create() is %d\n", rc);
	 	    exit(EXIT_FAILURE);
	 	}
     }

	/* Wait for All Producer Theads to Finish */
    for(t=0;t<numinputfiles;t++){
		pthread_join(producer_threads[t],NULL);
    }
    printf("All of the threads were completed!\n");
	
    /*Set doneWritingToQueue to TRUE so consumer threads know
    they can stop*/
    doneWritingToQueue = 1;

    /* Wait for All Consumer Theads to Finish */
     for(t=0;t<NUM_THREADS;t++){
		 pthread_join(consumer_threads[t],NULL);
     }
    printf("All of the threads were completed!\n");
	
    /*Free semaphore*/
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    sem_destroy(&sem_m);
    sem_destroy(&sem_results);

    /*Free queue*/
    queue_cleanup(&q);   
    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}
