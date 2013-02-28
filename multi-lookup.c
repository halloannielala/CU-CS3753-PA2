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


Description: This is a 
 */
#include "multi-lookup.h"

FILE* outputfp = NULL;
queue q;
int doneWritingToQueue = 0;
int processIsDone = 0;
sem_t sem_full;
sem_t sem_empty;
sem_t sem_m;
sem_t sem_results;
sem_t sem_producers_done;
sem_t sem_process_done;

/* Function for Each Thread to Run */
void* Producer(void* fn)
{
    /* Setup Local Vars and Handle void* */
    char* filename = fn;
    char* payload;
    char hostname[MAX_NAME_LENGTH];
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
        // sem_getvalue(&sem_empty, &valp);
        // printf("Thread %s INPUTFS empty %d",filename, valp);
        // sem_getvalue(&sem_full, &valp);
        // printf(" full %d\n",valp);

        sem_wait(&sem_empty);
        sem_wait(&sem_m);
        // sem_getvalue(&sem_empty, &valp);
        // printf("VALUE of EMPTY Semaphore after entering lock %d\n",valp);

        payload = malloc(MAX_NAME_LENGTH*sizeof(char));
        strcpy(payload, hostname);
		/*Add name to queue*/
        if(queue_push(&q, payload) == QUEUE_FAILURE){
            fprintf(stderr,
                "error: queue_push failed!\n"
                "Thread %s, Name: %s\n",
                filename, hostname);
        }
        printf("PUSH %s to queue\n", hostname);

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
    char hostname[MAX_NAME_LENGTH];
    char* holder_variable = NULL;
    long* tid = threadid;
    //char errorstr[SBUFSIZE];
    char firstipstr[MAX_IP_LENGTH];
    int valp;
    int val_m;
    int complete;
    int isEmpty;

    while(1){
        // if(sem_getvalue(&sem_producers_done, &valp) == 0 && valp < 1 
        //     && sem_getvalue(&sem_full, &val_m) == 0 && val_m < 1){
        //     break;
        // }
        complete = 0;
        int i = 0;
        if(doneWritingToQueue){
            printf("1st Thread %ld is done\n", *tid);
            for(i = 0; i < QUEUE_SIZE;i++){
                sem_post(&sem_full);    
            }
            return NULL;
        }
        // sem_wait(&sem_producers_done);
        // if(doneWritingToQueue == 2){
        //     complete = 2;
        // }else if (doneWritingToQueue == 1){
        //     complete = 1;
        // }
        // sem_post(&sem_producers_done);
        // //sem_wait(&sem_m);
        // if(queue_is_empty(&q)){
        //     complete++;
        // }
        // // sem_post(&sem_m);
      
        // if(complete >= 2) return NULL;
        
    	/*Decrement empty stderremaphore and acquire queue lock*/
        sem_wait(&sem_full);
        sem_wait(&sem_m);

    	/*Get a name*/
        if(!(queue_is_empty(&q))){
            if((holder_variable = (char*)queue_pop(&q)) == NULL){
                fprintf(stderr,
                    "error: queue_pop failed!\n"
                    "Threadid: %p\n",
                    threadid);
            }
            strcpy(hostname, holder_variable);
            // if(strcmp(hostname, ))
            printf("POP%ld: %s from queue\n", *tid, hostname);


            /* Lookup hostname and get IP string */
            if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
                             == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(firstipstr, "", sizeof(firstipstr));  
            }
            free(holder_variable);
        }else{
            if(doneWritingToQueue){
                printf("1st Thread %ld is done\n", *tid);
                sem_post(&sem_full);
                sem_post(&sem_m);
                return NULL;
            }
        }
    	/*Unlock queue*/
        sem_post(&sem_m);
        sem_post(&sem_empty);

        /*Lock file*/
        sem_wait(&sem_results);
        /* Write to Output File */
        printf("%s,%s %ld to file\n", hostname, firstipstr, *tid);
    	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
    	/*Unlock file*/
        sem_post(&sem_results);

        /*If producers are done and queue is empty, break
        out of while loop to finish this thread*/
        // if(doneWritingToQueue){
        //     printf("2nd Thread %ld is done\n", *tid);
        //     return NULL;
        // }
    }
    printf("LAST Thread %ld is done\n", *tid);
	return NULL;
}


int main(int argc, char* argv[]){

    /* Local Vars */
    
    // char hostname[SBUFSIZE];
    // char errorstr[SBUFSIZE];
    // char firstipstr[INET6_ADDRSTRLEN];
    //int i;
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
    if(sem_init(&sem_process_done, 0, 1) == -1){
        fprintf(stderr, "Error creating sem_process_done\n");
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
    printf("All of the producer threads were completed!\n");
	
    /*Set doneWritingToQueue to 1 so consumer threads know
    they can stop*/
    sem_wait(&sem_producers_done);
    doneWritingToQueue = 1;
    sem_post(&sem_producers_done);


    /* Wait for All Consumer Theads to Finish */
     for(t=0;t<NUM_THREADS;t++){
		 pthread_join(consumer_threads[t],NULL);
     }
    printf("All of the comsumer threads were completed!\n");
	
    /*Free semaphore*/
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
