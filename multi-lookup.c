/*Pthread version
 */
#include <pthread.h>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

int control; /*Number of producer threads */

typedef struct producer_args_s{
    queue* q;
    FILE* file;
    pthread_mutex_t* mutex;
}producer_args;

typedef struct consumer_args_s{
    queue* q;
    FILE* file;
    pthread_mutex_t* mutex;
}consumer_args;

void* Producer(void* args){
	producer_args* arg_p = args;
    char hostname[MAX_NAME_LENGTH];
    char strings[MAX_NAME_LENGTH];
    
    int i = 0;
    
    /* Read File and Process*/
    while(fscanf(arg_p->file, INPUTFS, hostname) > 0){
	    
	    printf("Read file in Producer OK \n");
	    
	    /* Prepare to write */
	    strcpy(strings,hostname);   
		
		printf("Prepare to write in queue OK \n");
		
		/* Wait for queue is ready */
		while(queue_is_full(arg_p->q)){
			int j = 1 + (int)( 100.0 * rand() / ( RAND_MAX + 1.0 ) );
			usleep(j);
		}
		
		printf("wait for queue and sleep OK \n");
		
		/*Write to queue */
		pthread_mutex_lock(arg_p->mutex);
		queue_push(arg_p->q, strings);
		i++;
		pthread_mutex_unlock(arg_p->mutex);
		
		printf("producer: put %d\n",i);
		
		printf("push in queue OK \n");
	}
	fclose(arg_p->file);
	
	printf("producer finally produced %d items\n",i);

	/*minus 1 thread number before it quit */
	pthread_mutex_lock(arg_p->mutex);
	control-=1;
	printf("Quit producer OK \n");
	pthread_mutex_unlock(arg_p->mutex);
	
	return NULL;
}

void* Consumer(void* args){
	consumer_args* arg_c = args;
    char hostname[MAX_NAME_LENGTH];
    char strings[MAX_NAME_LENGTH];
    char firstipstr[MAX_IP_LENGTH];
    
    int i=0;
    
	while(1){
		/* if producer thread is runing and queue is not empty */
		if(!queue_is_empty(arg_c->q)){
		
		printf("Test queue in consumer OK \n");
		/*We can strart to manipulate critical section */	
		pthread_mutex_lock(arg_c->mutex);  
	
		strcpy(hostname,queue_pop(arg_c->q));
		printf("Copy srting down in consumer OK \n");
			/* Lookup hostname and get IP string */
			if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
				== UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", hostname);
				strncpy(firstipstr, "", sizeof(firstipstr));
			}
			
		/* Write */
		i++;
		
	    strcpy(strings,hostname);
        strcat(strings,","); 
		strcat(strings,firstipstr);
		fprintf(arg_c->file, "%s\n", strings);
		
		printf("consumer: got %d\n",i);
		printf("Write in file in consumer OK \n");
		
		pthread_mutex_unlock(arg_c->mutex);
		}
		
		if((!control)&&(queue_is_empty(arg_c->q))){
			return NULL;
			printf("Quit from consumer OK \n");
			printf("consumer consumed %d items\n",i);
		}
	}
	return NULL;
}

int main(int argc, char* argv[]){
	
	/* Setup Local Vars */
    int rc1, rc2; /* test if it is correct */
    long t;
    int num_threads = argc-2;
    pthread_t pthreads[MAX_RESOLVER_THREADS];
    pthread_t cthreads[MAX_RESOLVER_THREADS];
    producer_args arg_p[num_threads];
    consumer_args arg_c[num_threads];
    pthread_mutex_t mutex;
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;   
    char errorstr[MAX_NAME_LENGTH];
    
    control=num_threads;
    
    printf("Setup OK \n");
    
    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }
    
     /* initialize queue */
     queue q;
     int q_size = 50;
     if(queue_init(&q, q_size) == QUEUE_FAILURE){
		fprintf(stderr,
		"error: queue_init failed!\n");
     }
     
     printf("Queue OK \n");

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

	printf("Output file OK \n");
	 /*create threads*/
    for(t=0;t<num_threads;t++){
	
        inputfp = fopen(argv[t+1], "r");
         if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", argv[t+1]);
	    perror(errorstr);
            queue_cleanup(&q);
	    return EXIT_FAILURE;
        }
        
        arg_c[t].q = &q;
        arg_c[t].file = outputfp;
  
        arg_p[t].q = &q;
        arg_p[t].file = inputfp;

        pthread_mutex_init(&mutex, NULL);     
        pthread_mutex_unlock(&mutex); 
        arg_c[t].mutex = &mutex;
        arg_p[t].mutex = &mutex;

        printf("In main: creating consumer thread %ld\n", t);
        printf("In main: creating producer thread %ld\n", t);
        
        rc1 = pthread_create(&(pthreads[t]), NULL, Producer, &(arg_p[t]));
		if (rc1){
			printf("ERROR; return code from pthread_create() is %d\n", rc1);
			exit(EXIT_FAILURE);
		}
        
		rc2 = pthread_create(&(cthreads[t]), NULL, Consumer, &(arg_c[t]));
		if (rc2){
			printf("ERROR; return code from pthread_create() is %d\n", rc2);
			exit(EXIT_FAILURE);
		}
		printf("Creat thread OK \n");
	}
	
	/* Wait for All Theads to Finish */
	for(t=0;t<num_threads;t++){
        pthread_join(cthreads[t],NULL);
    }
    for(t=0;t<num_threads;t++){
		pthread_join(pthreads[t],NULL);
    }
    
	printf("Wait for thread OK \n");
	
    fclose(outputfp);
   
    printf("All of the threads were completed!\n");
    queue_cleanup(&q);

    return 0;
 
}
