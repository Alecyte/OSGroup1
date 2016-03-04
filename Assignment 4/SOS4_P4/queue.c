////////////////////////////////////////////////////////
// A queue (of process PCB addresses) implementation
// A queue will be made up of an array of PCB addresses;
// memory for this array is allocatd on first use of the
// queue; there will be exactly 1 page for this array, so
// Q_MAXSIZE should be decided accordingly, i.e. <=1024

#include "kernel_only.h"

extern PDE *k_page_directory;

/*** Initialize a queue ***/
void init_queue(QUEUE *q) {
	q->data = NULL; // array not allocated yet
	q->head = 0;
	q->count = 0;
}

/*** Add to end of queue ***/
// Returns the index in the queue array where item is added.
// TODO: what if queue is full or not enough memory for data
uint32_t enqueue(QUEUE *q, PCB *p) {
	uint32_t loc;

	// we will allocate space for items in queue on first use
	if (q->data == NULL) { 
		// allocate memory to hold queue data
		q->data = (uint32_t *)alloc_kernel_pages(1);
	}

	loc = (q->head + q->count) % Q_MAXSIZE;
	q->data[loc] = (uint32_t)p;
	q->count++;
	return loc;
}

/*** Remove from head of queue ***/
// Returns NULL if queue is empty
PCB *dequeue(QUEUE *q) {
	PCB *ret;

	do {
		if (q->count == 0) return NULL;

		ret = (PCB *)q->data[q->head];
		q->count--;
		q->head = (q->head + 1) % Q_MAXSIZE;

	} while (ret == (PCB *)Q_ITEM_REMOVED);
	return ret;
}

/*** Replace item in queue ***/
// Stores Q_ITEM_REMOVED to mark item as removed.
void remove_queue_item(QUEUE *q, uint32_t loc) {
	if (q->data != NULL) {
		q->data[loc] = (uint32_t)Q_ITEM_REMOVED;
	}
}

/*** Return page allocated for queue ***/
// Do not call free_queue without calling init_queue
void free_queue(QUEUE *q) {
	if (q->data != NULL) { 
		dealloc_page((void *)q->data, k_page_directory);
	}
}

/*** Print queue ***/
// Print queue from head to tail
void print_queue(QUEUE *q) {
	int i;

	sys_printf("Index\tPCB\n\n");
	for (i=0; i<q->count; i++) {
		sys_printf("%d\t%x\n",i,q->data[(q->head + i) % Q_MAXSIZE]);
	}
}
