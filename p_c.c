#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define BUFFER_SIZE 5
#define PRODUCER_AMOUNT 8
#define CONSUMER_AMOUNT 3

typedef struct {
	int items[BUFFER_SIZE];
	size_t last_in_index;
	size_t last_out_index;
	pthread_mutex_t mutex;
	sem_t full_spaces;
	sem_t empty_spaces;
} buffer_type;


pthread_t producers[PRODUCER_AMOUNT];
pthread_t consumers[CONSUMER_AMOUNT];

buffer_type buffer;

void *producer();
void *consumer();

/*---------------------------------------------------*/

void treat_semaphore_error(int errno) {
	if(errno != 0) exit(EXIT_FAILURE);
}

void down(int caller, int sem_name, int order, sem_t s) {
	int value; 
	sem_getvalue(&s, &value); 
	printf("The value of %s(%d)#%s is %d\n", (caller == 0) ? "producer" : "consumer", order, (sem_name == 0) ? "empty" : "full", value);

	treat_semaphore_error(sem_wait(&s));

	sem_getvalue(&s, &value); 
	printf("The value of %s(%d)#%s is %d\n", (caller == 0) ? "producer" : "consumer", order, (sem_name == 0) ? "empty" : "full", value);
}

void up(int caller, int sem_name, int order, sem_t s) {
	int value; 
	sem_getvalue(&s, &value); 
	printf("The value of %s(%d)#%s is %d\n", (caller == 0) ? "producer" : "consumer", order, (sem_name == 0) ? "empty" : "full", value);

	treat_semaphore_error(sem_post(&s));

	sem_getvalue(&s, &value); 
	printf("The value of %s(%d)#%s is %d\n", (caller == 0) ? "producer" : "consumer", order, (sem_name == 0) ? "empty" : "full", value);
}

/*---------------------------------------------------*/

void treat_mutex_error(int errno) {
	if(errno != 0) exit(EXIT_FAILURE);
}

void down_mutex(pthread_mutex_t s) {
  treat_mutex_error(pthread_mutex_trylock(&s));
}

void up_mutex(pthread_mutex_t s) {
  	treat_mutex_error(pthread_mutex_unlock(&s));
}



void show_thread_error(int error) {
	if(error) {
		printf("Error: %d\n", error);
	}
}

void create_producers() {
	int counter;
	for(counter = 0; counter < PRODUCER_AMOUNT; counter++) {
		int error = pthread_create(&producers[counter], NULL, producer, (void*)(intptr_t)counter);
		show_thread_error(error);
	}
}

void create_consumers() {
	int counter;

	for(counter = 0; counter < CONSUMER_AMOUNT; counter++) {
		int error = pthread_create(&consumers[counter], NULL, consumer, (void*)(intptr_t)counter);
		show_thread_error(error);
	}
}

void resume_producers(){
	int i;
	for (i = PRODUCER_AMOUNT-1; i >= 0 ; i--) {
		pthread_join(producers[i], NULL);
	}
}

void resume_consumers(){
	int i;
	for (i = CONSUMER_AMOUNT-1; i >= 0; i--) {
		pthread_join(consumers[i], NULL);
	}
}

/*---------------------------------------------------*/

void initialize_buffer() {
	int i = 0;
	for(i=0; i < BUFFER_SIZE; i++) {
		buffer.items[i]=0;
	}
	buffer.last_in_index = 0;
	buffer.last_out_index = 0;

	pthread_mutex_init(&(buffer.mutex),NULL);
	sem_init(&(buffer.full_spaces), 1, 0);
	sem_init(&(buffer.empty_spaces), 1, BUFFER_SIZE);

	int value; 

	sem_getvalue(&buffer.empty_spaces, &value);
	printf("empty initialized with %d\n", value);

	sem_getvalue(&buffer.full_spaces, &value);
	printf("full initialized with %d\n", value);

	printf("\n\n");
}

void push(int item, int id) {
	int index = buffer.last_in_index;

	buffer.items[index] = item;
	buffer.last_in_index = (index+1)%BUFFER_SIZE;

	printf("Producer %d - [%d] => %d - %d --> %d\n", id, index, item, index, (int)buffer.last_in_index);
}

int pop(int id) {
	int index = buffer.last_out_index;

	int response = buffer.items[index];
	buffer.last_out_index = (index+1)%BUFFER_SIZE;

	printf("Consumer %d - [%d] => %d - %d --> %d\n", id, index, response, index, (int)buffer.last_out_index);

	return response;
}

/*---------------------------------------------------*/

int produce() {
	return 1+rand()%9;
}

void *producer(void *order) {

	while(1) {
		sleep(2);
		int number = produce();

		down(0, 0,(int)(intptr_t)order, buffer.empty_spaces);
		down_mutex(buffer.mutex);
		push(number, (int)(intptr_t)order);
		up_mutex(buffer.mutex);
		up(0, 1,(int)(intptr_t)order, buffer.full_spaces);
	}
	return NULL;
}

void *consumer(void *order) {
	while(1) {
		sleep(2);
		down(1, 1, (int)(intptr_t)order, buffer.full_spaces);
		down_mutex(buffer.mutex);
		int number = pop((int)(intptr_t)order);
		up_mutex(buffer.mutex);
		up(1, 0, (int)(intptr_t)order, buffer.empty_spaces);
	}
	return NULL;
}

/*---------------------------------------------------*/


int main(void) {
	initialize_buffer();
	srand( (unsigned)time(NULL) );
	create_producers();
	create_consumers();
	resume_producers();
	resume_consumers();
}