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

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define TEST_SIZE 10

int main(int argc, char* argv[]){
	/* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    int i;
    
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
    /* Setup local vars */
    queue q;
    const int qSize = TEST_SIZE;
    int* payload_in[TEST_SIZE];
    int* payload_out[TEST_SIZE];
    /* Setup payload_in as int* array from
     * 0 to TEST_SIZE-1 */
    for(i=0; i<TEST_SIZE; i++){
		payload_in[i] = malloc(sizeof(*(payload_in[i])));
		*(payload_in[i]) = i;
    }
    /* Setup payload_out as int* array of NULL */
    for(i=0; i<TEST_SIZE; i++){
		payload_out[i] = NULL;
    }
    if(queue_init(&q, qSize) == QUEUE_FAILURE){
		fprintf(stderr, "error: queue_init failed!\n");
    }


    // Create Request Thread Pool to read name files


    // Create Resolver Thread Pool to read from queue and resolve dns


    // DONT DO FROM HERE==============================
    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++){
	
		/* Open Input File */
		inputfp = fopen(argv[i], "r");
		if(!inputfp){
		    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
		    perror(errorstr);
		    break;
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
		    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
		}

		/* Close Input File */
		fclose(inputfp);
    }
    //DONT DO ABOVE HERE ==============================

    /* Close Output File */
    fclose(outputfp);

    /* Cleanup Queue */
    queue_cleanup(&q);

    /* Cleanup payload_in */
    for(i=0; i<TEST_SIZE; i++){
	free(payload_in[i]);
    }

    return EXIT_SUCCESS;
}
