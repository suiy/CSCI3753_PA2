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


#define MAX_SIZE 500
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue q;
int control;

typedef struct producer_args_s{
    FILE* file;
    pthread_mutex_t* mutex;
}producer_args;

typedef struct consumer_args_s{
    FILE* file;
    pthread_mutex_t* mutex;
}consumer_args;

void print_q(queue* q){
	int a;
	char *b;
	for (a=q->front;a<q->rear;a++){
		b = q->array[a].payload;
		printf("%s\n",b);
		}
	}

void* Producer(void* args){
	
	// Set pointer to l=ocal pointer
	producer_args* arg_p = (producer_args*) args;
	
	// Temp buffer
    char hostname[MAX_NAME_LENGTH];
   // printf("2\n");
    // Read File and Process //
    while(fscanf(arg_p->file, INPUTFS, hostname)>0){
	//printf("3\n");
		// Test queue is full
		pthread_mutex_lock(arg_p->mutex);
		int full = queue_is_full(&q);
		pthread_mutex_unlock(arg_p->mutex);
		//printf("4\n");
		while(full){
			int j = 1 + (int)( 100.0 * rand() / ( RAND_MAX + 1.0 ) );
			usleep(j);
			printf("queue is full, thread is sleeping \n");
			pthread_mutex_lock(arg_p->mutex);
			full = queue_is_full(&q);
			pthread_mutex_unlock(arg_p->mutex);
			//printf("5\n");
			}

		// Push hostname in our queue
		pthread_mutex_lock(arg_p->mutex);
		//printf("6\n");
		
		// **may need free
		if(!queue_push(&q, hostname))
		{
			printf("after push, queue is\n");
			print_q(&q);
			//printf("7\n");
		}

		pthread_mutex_unlock(arg_p->mutex);
	}
	printf("8\n");
	// Close input file
	fclose(arg_p->file);
	
	// Minus threads number
	pthread_mutex_lock(arg_p->mutex);
	control-=1;
	printf("Quit producer OK \n");
	//printf("9\n");
	pthread_mutex_unlock(arg_p->mutex);
	return NULL;
}

void* Consumer(void* args){
	// Set local variable and buffer
	consumer_args* arg_c = (consumer_args *) args;
    char hostname[MAX_NAME_LENGTH];
    char strings[MAX_NAME_LENGTH];
    char firstipstr[MAX_IP_LENGTH];
    int is_empty;

    //printf("21\n");
    
	while(1){
		
		// Test producer thread is runing and queue is not empty
		pthread_mutex_lock(arg_c->mutex);
		is_empty = !queue_is_empty(&q);
		//printf("22\n");
		
		// If it is not empty, then pop out and write
		if(is_empty){	 
		//printf("23\n");
		
		char* a = (char *) queue_pop(&q);
		
		//printf("Special error\n");
		//printf("%s \n", a);
		strcpy(hostname,a);
		// ****may need free
		//printf("24\n");

		printf("after pop, queue is\n");
		print_q(&q);

		if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
			== UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
			printf("25\n");
		}
			
		/* Write */
		
	    strcpy(strings,hostname);
        strcat(strings,","); 
		strcat(strings,firstipstr);
		//printf("26\n");
		
		printf ("***********************from consumer %s\n", strings);
		
		fprintf(arg_c->file, "%s\n", strings);
		//printf("27\n");
		
		pthread_mutex_unlock(arg_c->mutex);
		}
		else {pthread_mutex_unlock(arg_c->mutex);}
		
		pthread_mutex_lock(arg_c->mutex);
		if((!control)&&(queue_is_empty(&q))){
			pthread_mutex_unlock(arg_c->mutex);
			return NULL;
			printf("Quit from consumer OK \n");
			//printf("28\n");
		}
		else {pthread_mutex_unlock(arg_c->mutex);}
	}
	return NULL;
}

int main(int argc, char* argv[]){
	
	// Set up local variable
	int t;
	int num_threads = argc-2;
	control = num_threads;
    pthread_t pthreads[MAX_RESOLVER_THREADS];
    pthread_t cthreads[MAX_RESOLVER_THREADS];
    
    producer_args arg_p[num_threads];
    consumer_args arg_c[num_threads];
    printf("31\n");
	
    pthread_mutex_t mutex;
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;   
    char errorstr[MAX_NAME_LENGTH];
    
    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }
    
     /* initialize queue */
     int q_size = 50;
     if(queue_init(&q, q_size) == QUEUE_FAILURE){
		fprintf(stderr,
		"error: queue_init failed!\n");
     }
     
    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }
    
    // Start to create threads
	pthread_mutex_init(&mutex, NULL);     
	pthread_mutex_unlock(&mutex); 

    for(t=0;t<num_threads;t++){
		printf("33\n");
        inputfp = fopen(argv[t+1], "r");
         if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File: %s", argv[t+1]);
	    perror(errorstr);
            queue_cleanup(&q);
	    return EXIT_FAILURE;
        }
  
        arg_c[t].file = outputfp;
        arg_p[t].file = inputfp;

        arg_c[t].mutex = &mutex;
        arg_p[t].mutex = &mutex;

        printf("In main: creating consumer thread %d\n", t);
        printf("In main: creating producer thread %d\n", t);
        
        int rc1 = pthread_create(&(pthreads[t]), NULL, Producer, &(arg_p[t]));
		if (rc1){
			printf("ERROR; return code from pthread_create() is %d\n", rc1);
			exit(EXIT_FAILURE);
		}
		
		printf("34\n");
		
		int rc2 = pthread_create(&(cthreads[t]), NULL, Consumer, &(arg_c[t]));
		if (rc2){
			printf("ERROR; return code from pthread_create() is %d\n", rc2);
			exit(EXIT_FAILURE);
		}
		printf("Creat thread OK \n");
	
	}

	 //Wait for All Theads to Finish */
    
    for(t=0;t<num_threads;t++){
		pthread_join(pthreads[t],NULL);
    }
    
    printf("35\n");

	for(t=0;t<num_threads;t++){
        pthread_join(cthreads[t],NULL);
    }

	//free memory
	queue_cleanup(&q);

	//close output
    fclose(outputfp);

    printf("All of the threads were completed!\n");
    return 0;
 
}
