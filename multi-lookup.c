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


Description: This is a multithreaded requester-resolver
DNS Resolution Engine.

Determines number of threads to use automatically.
number_of_threads = number_available_cores/(1 - blocking_coefficient)
Since this is an I/O intensive application, use blocking_coefficient
of 0.9


 */
#include "multi-lookup.h"

int VERBOSE = 0;
FILE* outputfp = NULL;
queue q;
int doneWritingToQueue = 0;
int processIsDone = 0;
sem_t sem_full;
sem_t sem_empty;
sem_t sem_m;
sem_t sem_results;
sem_t sem_producers_done;

/* Function for Each Thread to Run */
void* Producer(void* fn)
{
    /* Setup Local Vars and Handle void* */
    char* filename = fn;
    char* payload;
    char hostname[MAX_NAME_LENGTH];
    char errorstr[SBUFSIZE];
    FILE* inputfp = NULL;

	/* Open Input File */
	inputfp = fopen(filename, "r");
	if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", filename);
	    perror(errorstr);
	}

	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		/*Decrement empty semaphore and acquire queue lock*/
        sem_wait(&sem_empty);
        sem_wait(&sem_m);

        /*Malloc the space to add to the queue*/
        payload = malloc(MAX_NAME_LENGTH*sizeof(char));
        strcpy(payload, hostname);
		
        /*Add name to queue*/
        if(queue_push(&q, payload) == QUEUE_FAILURE){
            fprintf(stderr,
                "error: queue_push failed!\n"
                "Thread %s, Name: %s\n",
                filename, hostname);
        }
        if(VERBOSE){printf("PUSH %s to queue\n", hostname);}

	    /*Unlock queue and increment full semaphore*/
        sem_post(&sem_m);
        sem_post(&sem_full);
	}
    
	/* Close Input File */
	fclose(inputfp);

    /* Exit, Returning NULL*/
    return NULL;
}

void* Consumer(void* threadid){
    char hostname[MAX_NAME_LENGTH];
    char* holder_variable = NULL;
    long* tid = threadid;
    char firstipstr[MAX_IP_LENGTH];

    while(1){
        int i = 0;

        /*Check if the producers are done writing to the queue
        If they are, this means there is no reason to keep 
        other threads waiting. Increment the sem_full semaphore
        QUEUE_SIZE times so that none remain blocked*/
        if(doneWritingToQueue){
            if(VERBOSE){printf("1st Thread %ld is done\n", *tid);}
            for(i = 0; i < QUEUE_SIZE;i++){
                sem_post(&sem_full);    
            }
            return NULL;
        }
        
    	/*Increment full semaphore and acquire queue lock*/
        sem_wait(&sem_full);
        sem_wait(&sem_m);

    	/*If the queue is not empty, use holder variable to get 
        pointer to name*/
        if(!(queue_is_empty(&q))){
            if((holder_variable = (char*)queue_pop(&q)) == NULL){
                fprintf(stderr,
                    "error: queue_pop failed!\n"
                    "Threadid: %p\n",
                    threadid);
            }
            /*Copy name to hostname*/
            strcpy(hostname, holder_variable);
            if(VERBOSE){
                printf("POP%ld: %s from queue\n", *tid, hostname);
            }

            /* Lookup hostname and get IP string */
            if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
                             == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(firstipstr, "", sizeof(firstipstr));  
            }
            /*Free the holder variable that was malloced by the 
            producer*/
            free(holder_variable);
        }else{/*If the queue was empty, check to see if the 
            producers are done producing. If so, release the lock
            on the queue, increment full for goo measure, and exit*/
            if(doneWritingToQueue){
                if(VERBOSE){printf("1st Thread %ld is done\n", *tid);}
                sem_post(&sem_full);
                sem_post(&sem_m);
                return NULL;
            }
        }
    	/*Unlock queue and increment empty semaphore*/
        sem_post(&sem_m);
        sem_post(&sem_empty);

        /*Lock file*/
        sem_wait(&sem_results);
        /* Write to Output File. This only happens if the queue was
        not empty and you were able to extract something from it*/
        if(VERBOSE){
            printf("%s,%s %ld to file\n", hostname, firstipstr, *tid);
        }
    	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
    	/*Unlock file*/
        sem_post(&sem_results);
    }
    if(VERBOSE){printf("LAST Thread %ld is done\n", *tid);}
	return NULL;
}


int main(int argc, char* argv[]){
    /* Local Vars */
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN );
    //printf("numCPU %d\n", numCPU);
    int number_of_threads = numCPU/(1-BLOCKING_COEFFICIENT);
    int numinputfiles;
    pthread_t consumer_threads[number_of_threads];
    int rc;
    long t;
    long cpyt[NUM_THREADS];

    /*Initialize semaphores*/
    if(sem_init(&sem_full, 0, 0) == -1){
        fprintf(stderr, "Error creating sem_init\n");
    }
    if(sem_init(&sem_empty, 0, QUEUE_SIZE) == -1){
        fprintf(stderr, "Error creating sem_empty\n");
    }
    if(sem_init(&sem_m, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_m\n");
    }
    if(sem_init(&sem_results, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_results\n");
    }
    if(sem_init(&sem_producers_done, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_producers_done\n");
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

    /* Loop Through Input Files and Create Producer Threads */
    for(t=0; t<numinputfiles; t++){
        if(VERBOSE){
		  printf("In main: creating producer thread %ld, %s\n", t, argv[t+1]);
        }
		rc = pthread_create(&(producer_threads[t]), NULL, Producer, argv[t+1]);
		if (rc){
		    printf("ERROR; return code from pthread_create() is %d\n", rc);
		    exit(EXIT_FAILURE);
		}
	}	
    t = 0;
    /* Spawn NUM_THREADS Consumer threads */
    for(t=0;t<NUM_THREADS;t++){
        if(VERBOSE){
    	   printf("In main: creating consumer thread %ld\n", t);
        }
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
    if(VERBOSE){
        printf("All of the producer threads were completed!\n");
	}
    /*Set doneWritingToQueue to 1 so consumer threads know
    they can stop*/
    sem_wait(&sem_producers_done);
    doneWritingToQueue = 1;
    sem_post(&sem_producers_done);


    /* Wait for All Consumer Theads to Finish */
     for(t=0;t<NUM_THREADS;t++){
		 pthread_join(consumer_threads[t],NULL);
     }
    if(VERBOSE){
        printf("All of the comsumer threads were completed!\n");
	}

    /*Free semaphores*/
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    sem_destroy(&sem_m);
    sem_destroy(&sem_results);
    sem_destroy(&sem_producers_done);

    /*Free queue*/
    queue_cleanup(&q);   

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}
