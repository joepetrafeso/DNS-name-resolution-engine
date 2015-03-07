/*
 * File: util.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2012/02/01
 * Modify Date: 2012/02/01
 * Description:
 * 	This file contains declarations of utility functions for
 *      Programming Assignment 2.
 *  
 */

#include "util.h"

#define MAX_DNS_RESULTS 50

void addToDnsList(queue* q, char * ip){
	char * ipAddress;

	int i;
    for(i = 0; i < q->rear; i++){
    	if (!strcmp(ip,(char *) q->array[q->front + i].payload)){
    		return;
    	}
    }

	ipAddress = malloc(INET6_ADDRSTRLEN);
	strncpy(ipAddress, ip, INET6_ADDRSTRLEN);
	queue_push(q, ipAddress);
}

int dnslookup(const char* hostname, char* firstIPstr, int maxSize){
	return dnslookupall(hostname, firstIPstr, maxSize, NULL);
}

int dnslookupall(const char* hostname, char* firstIPstr, int maxSize, queue* q){

    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct in_addr* ipv4addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int addrError = 0;

    /* DEBUG: Print Hostname*/
	#ifdef UTIL_DEBUG
    	fprintf(stderr, "%s\n", hostname);
	#endif

   	// queue_init(&q, MAX_DNS_RESULTS);
   
    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
		fprintf(stderr, "Error looking up Address: %s\n",
			gai_strerror(addrError));
		return UTIL_FAILURE;
    }
    /* Loop Through result Linked List */
    for(result=headresult; result != NULL; result = result->ai_next){
		/* Extract IP Address and Convert to String */
		if(result->ai_addr->sa_family == AF_INET){
		    /* IPv4 Address Handling */
		    ipv4sock = (struct sockaddr_in*)(result->ai_addr);
		    ipv4addr = &(ipv4sock->sin_addr);
		    if(!inet_ntop(result->ai_family, ipv4addr,
				  ipv4str, sizeof(ipv4str))){
				perror("Error Converting IP to String");
				return UTIL_FAILURE;
			}

			#ifdef UTIL_DEBUG
			    fprintf(stdout, "bob%s\n", ipv4str);
			#endif
		    strncpy(ipstr, ipv4str, sizeof(ipstr));
		    ipstr[sizeof(ipstr)-1] = '\0';

		    if(q != NULL){
		    	addToDnsList(q, ipv4str);
		    }
		    
		}
		else if(result->ai_addr->sa_family == AF_INET6){
	    	/* IPv6 Handling */
			#ifdef UTIL_DEBUG
			    fprintf(stdout, "IPv6 Address: Not Handled\n");
			#endif
		    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
		    ipstr[sizeof(ipstr)-1] = '\0';
		}
		else{
	    	/* Unhandlded Protocol Handling */
			#ifdef UTIL_DEBUG
			    fprintf(stdout, "Unknown Protocol: Not Handled\n");
			#endif
		    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
		    ipstr[sizeof(ipstr)-1] = '\0';
		}
		/* Save First IP Address */
		if(result==headresult){
		    strncpy(firstIPstr, ipstr, maxSize);
		    firstIPstr[maxSize-1] = '\0';
		}
		
    }
    
    /* Cleanup */
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}