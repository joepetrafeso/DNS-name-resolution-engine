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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define TEST_SIZE 10
#define NUM_THREADS 10

// Thread Data Structures
struct RequestData{
    long threadNumber;
    FILE* inputFile;
    queue* q;
};
struct ResolveData{
    FILE* outputFile;
    queue* q;
};

// Thread Mutexes
pthread_mutex_t mutex;
pthread_mutex_t writeBlock;
pthread_mutex_t readBlock;
pthread_mutex_t queueBlock;
pthread_mutex_t reportBlock;

int requestThreadsComplete = 0;
int readCount = 0;
int debug = 1;


void* RequestThread(void* threadarg){
    struct RequestData * requestData;
    char hostname[SBUFSIZE];
    FILE* inputfp;
    queue* q;
    char* payload;
    long threadNumber;

    requestData = (struct RequestData *) threadarg;
    inputfp = requestData->inputFile;
    q = requestData->q;
    threadNumber = requestData->threadNumber;

    // Open Input File
    if(!inputfp){
        printf("Error Opening Input File");
        pthread_exit(NULL);
    }

    // Add to queue
    if (debug){
    	printf("File being read\n");
    }
    while(fscanf(inputfp, INPUTFS, hostname) > 0){
        int completed = 0;
        while(!completed){
            // Check if queue is full
            pthread_mutex_lock(&readBlock);
            pthread_mutex_lock(&writeBlock);
            if (!queue_is_full(q)){
                payload = malloc(SBUFSIZE);
                strncpy(payload, hostname, SBUFSIZE);

                if(debug){
                	printf("Adding Payload to Queue:%s\n", payload);
                }
                queue_push(q, payload);
                completed = 1;
            }
            // release queue
            pthread_mutex_unlock(&writeBlock);
            pthread_mutex_unlock(&readBlock);

            // Wait if not completed
            if (!completed){
                usleep((rand()%100)*100000);
            }
        }
        //release queue
    }
    fclose(inputfp);

    if(debug){
    	printf("Completing Request thread %ld\n", threadNumber);
	}
    return NULL;
}

void* ResolveThread(void* threadarg){
    struct ResolveData * resolveData;
    FILE* outputfp;
    char firstipstr[INET6_ADDRSTRLEN];
    char * hostname;
    queue* q;


    resolveData = (struct ResolveData *) threadarg;
    outputfp = resolveData->outputFile;
    q = resolveData->q;

    int emptyQueue = 0;
    while (!emptyQueue || !requestThreadsComplete){
    	int resolved = 0;
        pthread_mutex_lock(&readBlock);
        pthread_mutex_lock(&mutex);
        readCount++;
        if (readCount==1){
            pthread_mutex_lock(&writeBlock);
        }
        pthread_mutex_unlock(&mutex);
        pthread_mutex_unlock(&readBlock);

        // Pop off the queue one at a time
        pthread_mutex_lock(&queueBlock);
        emptyQueue = queue_is_empty(q);
        if(!emptyQueue){
            hostname = queue_pop(q);
            if (hostname != NULL){
            	if(debug){
            		printf("Reading From Queue: %s\n", hostname);
            	}
	            resolved = 1;
        	}
        }
        pthread_mutex_unlock(&queueBlock);

        pthread_mutex_lock(&mutex);
        readCount--;
        if (readCount == 0){
            pthread_mutex_unlock(&writeBlock);
        }
        pthread_mutex_unlock(&mutex);

        if(resolved){
        	queue dnsList;
        	queue_init(&dnsList, 50);
            if(dnslookupall(hostname, firstipstr, sizeof(firstipstr), &dnsList) == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(firstipstr, "", sizeof(firstipstr));
            }
            fprintf(outputfp, "%s", hostname);
    		char * element;
    		pthread_mutex_lock(&reportBlock);
            while( (element = (char *) queue_pop(&dnsList)) != NULL){
		    	fprintf(outputfp, ",%s", element);
		    	free(element);
		    }
		    fprintf(outputfp, "\n");
            pthread_mutex_unlock(&reportBlock);
		    free(hostname);
		    queue_cleanup(&dnsList);
        }

    }

    if(debug){
    	printf("Complete Resolve thread\n");
    }

    return NULL;
}


int main(int argc, char* argv[]){
	/* Local Vars */
    FILE* outputfp = NULL;

    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }
    if (argc > NUM_THREADS + 1){
    	fprintf(stderr, "Too many files: %d\n", (argc - 2));
    	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    // Build Queue
    queue q;
    const int qSize = TEST_SIZE;
    if(queue_init(&q, qSize) == QUEUE_FAILURE){
		fprintf(stderr, "error: queue_init failed!\n");
    }


    /////////////////////////////////////////////////
    // Create REQUEST Thread Pool
    /////////////////////////////////////////////////
    struct RequestData requestData[NUM_THREADS];
    pthread_t requestThreads[NUM_THREADS];
    int rc;
    long t;

    /* Spawn REQUEST threads */
    for(t=0; t<(argc-2) && t<NUM_THREADS; t++){
        requestData[t].q = &q;
        requestData[t].inputFile = fopen(argv[t+1], "r");
        requestData[t].threadNumber = t;
        if (debug){
        	printf("Creating REQUEST Thread %ld\n", t);
        }
		rc = pthread_create(&(requestThreads[t]), NULL, RequestThread, &(requestData[t]));
		if (rc){
		    printf("ERROR; return code from pthread_create() is %d\n", rc);
		    exit(EXIT_FAILURE);
		}
    }


    /////////////////////////////////////////////////
    // Create RESOLVE Thread Pool
    /////////////////////////////////////////////////
    struct ResolveData resolveData;
    int cpuCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t resolveThreads[cpuCoreCount];
    int rc2;
    long t2;

    resolveData.q = &q;
    resolveData.outputFile = outputfp;

    /* Spawn RESOLVE threads */

    for(t2=0; t2<cpuCoreCount; t2++){
    	if (debug){
    		printf("Creating RESOLVER Thread %ld\n", t2);
    	}
        rc2 = pthread_create(&(resolveThreads[t2]), NULL, ResolveThread, &resolveData);
        if (rc2){
            printf("ERROR; return code from pthread_create() is %d\n", rc2);
            exit(EXIT_FAILURE);
        }
    }
    //Join Threads to detect completion
    for(t=0; t<(argc-2) && t<NUM_THREADS; t++){
        pthread_join(requestThreads[t],NULL);
    }
    if (debug){
    	printf("All Request threads have FINISHED!\n");
    }
    requestThreadsComplete = 1;

    //Join Threads to detect completion
    for(t=0; t<cpuCoreCount; t++){
        pthread_join(resolveThreads[t],NULL);
    }
    if (debug){
    	printf("All Resolver threads have FINISHED!\n");
    }

    /* Close Output File */
    fclose(outputfp);

    /* Cleanup Queue */
    queue_cleanup(&q);

    return EXIT_SUCCESS;
}
