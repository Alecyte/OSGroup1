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
	int i;
	char c;

	SHARED_DATA *b = (SHARED_DATA *)smattach(SM_KEY, SM_READ_WRITE);
	if (b==NULL) {
		printf("No memory area to attach to.\n");
		return;
	}

	// TODO: Check that semaphore and mutex objects have been created

	mlock(b->mx_consumer_count);
	b->n_consumers++;
	munlock(b->mx_consumer_count);

	do {
		sdown(b->sem_full);
		mlock(b->mx_buffer);

		// begin critical section
   		c = b->slot[b->out];
		b->out = (b->out+1)%BUFFER_SIZE;
		if (c==0) {
			b->n_consumers--;
			munlock(b->mx_buffer);
			sup(b->sem_empty);
			break;
		}
		// end critical section

	  	munlock(b->mx_buffer);
		sup(b->sem_empty);

		printf("%c",c);
		sleep(300); // simulation: consumer using item
	} while (TRUE);

	sup(b->sem_done);
	smdetach();
  }
