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
    printf("File being read\n");
    while(fscanf(inputfp, INPUTFS, hostname) > 0){
        // printf("%s\n", hostname);
        int completed = 0;
        while(!completed){
            // Check if queue is full
            pthread_mutex_lock(&readBlock);
            pthread_mutex_lock(&writeBlock);
            if (!queue_is_full(q)){
                payload = malloc(SBUFSIZE);
                strncpy(payload, hostname, SBUFSIZE);
                printf("%s\n", payload);
                queue_push(q, payload);
                completed = 1;
            }
            // release queue
            pthread_mutex_unlock(&writeBlock);
            pthread_mutex_unlock(&readBlock);

            // Wait if not completed
            if (!completed){
                usleep((rand()%100)*10000);
                // TODO - Remove this when we get locks in place
                // break;
            }
        }
        //release queue
    }
    fclose(inputfp);
    printf("Completing Request thread %ld\n", threadNumber);
    return NULL;
    // pthread_exit(NULL);
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

    int resolved = 0;
    while (!resolved || !requestThreadsComplete){
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
        resolved = queue_is_empty(q);
        if(!resolved){
            hostname = queue_pop(q);
        }
        pthread_mutex_unlock(&queueBlock);

        pthread_mutex_lock(&mutex);
        readCount--;
        if (readCount == 0){
            pthread_mutex_unlock(&writeBlock);
        }
        pthread_mutex_unlock(&mutex);

        if(!resolved){
            if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(firstipstr, "", sizeof(firstipstr));
            }
            printf("Host:%s\n", hostname);
            printf("IP:%s\n", firstipstr);

            /* Write to Output File */
            pthread_mutex_lock(&reportBlock);
            fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
            pthread_mutex_unlock(&reportBlock);
        }

    }

    printf("Complete Resolve thread\n");

    return NULL;
    // pthread_exit(NULL);
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
    for(t=1; t<(argc-1) && t<NUM_THREADS; t++){
        requestData[t].q = &q;
        requestData[t].inputFile = fopen(argv[t], "r");
        requestData[t].threadNumber = t;
		printf("Creating REQUEST Thread %ld\n", t);
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
    pthread_t resolveThreads[NUM_THREADS];
    int rc2;
    long t2;

    resolveData.q = &q;
    resolveData.outputFile = outputfp;

    /* Spawn RESOLVE threads */
    for(t2=1; t2<3; t2++){

        printf("Creating RESOLVER Thread %ld\n", t2);
        rc2 = pthread_create(&(resolveThreads[t2]), NULL, ResolveThread, &resolveData);
        if (rc2){
            printf("ERROR; return code from pthread_create() is %d\n", rc2);
            exit(EXIT_FAILURE);
        }
    }
    //Join Threads to detect completion
    for(t=1; t<(argc-1) && t<NUM_THREADS; t++){
        pthread_join(requestThreads[t],NULL);
    }
    requestThreadsComplete = 1;

    //Join Threads to detect completion
    for(t=1; t<NUM_THREADS; t++){
        pthread_join(resolveThreads[t],NULL);
    }

    /* Close Output File */
    fclose(outputfp);

    /* Cleanup Queue */
    queue_cleanup(&q);

    return EXIT_SUCCESS;
}
