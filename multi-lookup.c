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

#include "util.h"

#define MINARGS 3
#define USAGE "<inputFilePath1> (other input filepaths) <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

/* Function for Each Thread to Run */
void* Producer(void* fn)
{
    /* Setup Local Vars and Handle void* */
    char* filename = fn;
    long t;
    long numprint = 3;
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    FILE* inputfp = NULL;

    /* Print hello numprint times */
    for(t=0; t<numprint; t++)
	{
	    printf("Hello World! It's me, thread %s! "
		   "This is printout %ld of %ld\n",
		   filename, (t+1), numprint);

	    /* Sleep for 1 to 2 Seconds */
	    usleep((rand()%100)*10000+1000000);
	}

	/* Open Input File */
	inputfp = fopen(filename, "r");
	if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", filename);
	    perror(errorstr);
	}

	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
	
	    /* Lookup hostname and get IP string */
	    if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
	       == UTIL_FAILURE){
		fprintf(stderr, "dnslookup error: %s\n", hostname);
		strncpy(firstipstr, "", sizeof(firstipstr));
	    }

	     /* Write to Output File */
	    printf("Thread %s %s,%s\n", filename, hostname, firstipstr);
	}
    
	/* Close Input File */
	fclose(inputfp);

    /* Exit, Returning NULL*/
    return NULL;
}



int main(int argc, char* argv[]){

    /* Local Vars */
    FILE* outputfp = NULL;
    // char hostname[SBUFSIZE];
    // char errorstr[SBUFSIZE];
    // char firstipstr[INET6_ADDRSTRLEN];
    int i;
    int numinputfiles;
    
    /* Setup Local Vars */
    int rc;
    long t;
    //long cpyt[NUM_THREADS];
    
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

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

    /* Loop Through Input Files */
    for(i=0; i<numinputfiles; i++){
		printf("In main: creating thread %ld\n", t);
		rc = pthread_create(&(producer_threads[i]), NULL, Producer, argv[i+1]);
		if (rc){
		    printf("ERROR; return code from pthread_create() is %d\n", rc);
		    exit(EXIT_FAILURE);
		}
	}	

	/* Wait for All Theads to Finish */
    for(t=0;t<numinputfiles;t++){
		pthread_join(producer_threads[t],NULL);
    }
    printf("All of the threads were completed!\n");
	
	
	//     /* Write to Output File */
	//     fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
	// }

	

    /* Close Output File */
    //fclose(outputfp);

    return EXIT_SUCCESS;
}
