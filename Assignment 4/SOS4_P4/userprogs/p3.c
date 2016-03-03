#include "../lib.h"

#define SM_KEY 		36
#define BUFFER_SIZE 	5

typedef struct {
	char slot[BUFFER_SIZE];
	int in, out;
	int n_consumers;
	mutex_t mx_buffer;
	mutex_t mx_consumer_count;
	sem_t sem_empty;
	sem_t sem_full;
	sem_t sem_done;
} SHARED_DATA;

void main() {
	char str[] = "It looked like a good thing: but wait till I tell you. We were down South, in Alabama--Bill Driscoll and myself-when this kidnapping idea struck us. It was, as Bill afterward expressed it, \"during a moment of temporary mental apparition\"; but we didn't find that out till later.\n";

	int i = 0;
	SHARED_DATA *b = (SHARED_DATA *)smcreate(SM_KEY, sizeof(SHARED_DATA));

	if (b==NULL) {
		printf("Unable to create shared memory area.\n");
		return;
	}

	b->sem_empty = screate(BUFFER_SIZE);
	b->sem_full = screate(0);
	b->sem_done = screate(0);

	if (b->sem_empty == 0 || b->sem_full == 0 || b->sem_done == 0) { 
		smdetach();
		printf("Unable to create semaphore objects.\n");
		return;
	}

	b->mx_buffer = mcreate();
	b->mx_consumer_count = mcreate();

	if (b->mx_buffer == 0 || b->mx_consumer_count == 0) {
		smdetach();
		printf("Unable to create mutex objects.\n");
		return;
	}

	b->in = 0; b->out = 0; b->n_consumers = 0;
	printf("Producing items...consumers can run now.\n");

	while(str[i]!=0) {
		sleep(50); // simulation: producer producing next item
	  	sdown(b->sem_empty);
		// begin critical section
   		b->slot[b->in] = str[i];
		b->in = (b->in+1)%BUFFER_SIZE;
		// end critical section
	  	sup(b->sem_full);
		i++;
	} 

	printf("\nDone producing...waiting for consumers to end.\n");

	mlock(b->mx_consumer_count);
	int alive;
	
	do {
		mlock(b->mx_buffer);
		alive = b->n_consumers;
		munlock(b->mx_buffer);
		
		if (alive > 0) { // consumers are alive
			sdown(b->sem_empty);
			b->slot[b->in] = 0; // send END signal to consumer
			b->in = (b->in+1)%BUFFER_SIZE;
			sup(b->sem_full);
			sdown(b->sem_done);
		}
	} while (alive>0);
	
	printf("\nShutters down!\n");

	sdestroy(b->sem_full);
	sdestroy(b->sem_empty);
	sdestroy(b->sem_done);
	mdestroy(b->mx_buffer);
	mdestroy(b->mx_consumer_count);
	smdetach();
}
