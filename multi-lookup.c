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

struct RequestData{
    FILE* inputFile;
    queue* q;
};
struct ResolveData{
    FILE* outputFile;
    queue* q;
};


void* ReadFile(void* threadarg){
    struct RequestData * requestData;
    char hostname[SBUFSIZE];
    FILE* inputfp;
    queue* q;
    char* payload;

    requestData = (struct RequestData *) threadarg;
    inputfp = requestData->inputFile;
    q = requestData->q;
    printf("queue location %p\n", q);

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
        // grab queue
        while(completed == 0){
            // Check if queue is full
            if (!queue_is_full(q)){
                payload = malloc(SBUFSIZE);
                strncpy(payload, hostname, SBUFSIZE);
                printf("%s\n", payload);
                queue_push(q, &payload);
                completed = 1;
            }
            else{
                // release queue
                break;
                usleep((rand()%100)*10000+1000000);
                // grab queue
            }
        }
        //release queue
    }
    fclose(inputfp);
    return NULL;
    // pthread_exit(NULL);
}


int main(int argc, char* argv[]){
	/* Local Vars */
    FILE* outputfp = NULL;
    char firstipstr[INET6_ADDRSTRLEN];

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

    // Create Request Thread Pool to read name files
    /* Setup Local Vars */
    struct RequestData requestthreads[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    /* Spawn REQUEST threads */
    for(t=1; t<(argc-1) && t<NUM_THREADS; t++){
        requestthreads[t].q = &q;
        requestthreads[t].inputFile = fopen(argv[t], "r");
		printf("Creating REQUEST Thread %ld\n", t);
		rc = pthread_create(&(threads[t]), NULL, ReadFile, &(requestthreads[t]));
		if (rc){
		    printf("ERROR; return code from pthread_create() is %d\n", rc);
		    exit(EXIT_FAILURE);
		}
    }

    //Join Threads to detect completion
    for(t=0;t<NUM_THREADS;t++){
        pthread_join(threads[t],NULL);
    }
    usleep(1000000);

    while (queue_is_empty(&q) == 0){
        char * test = malloc(SBUFSIZE);
        test = queue_pop(&q);
        // if(dnslookup(test, firstipstr, sizeof(firstipstr))
        //    == UTIL_FAILURE){
        //     fprintf(stderr, "dnslookup error: %s\n", test);
        //     strncpy(firstipstr, "", sizeof(firstipstr));
        // }
        printf("%c test\n", *test);

    }

    /* Close Output File */
    fclose(outputfp);

    /* Cleanup Queue */
    queue_cleanup(&q);

    /* Cleanup payload_in */
 //    for(i=0; i<TEST_SIZE; i++){
	// free(payload_in[i]);
 //    }

    return EXIT_SUCCESS;
}
