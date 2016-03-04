///////////////////////////////////////////////////////
// Semaphore implementation
// This implementation provides SEM_MAXNUMBER semaphores
// for use by user processes; the semaphore number is specified 
// using an 8-bit number (called key), which restricts us to 
// have up to 256 semaphore variables
// DO NOT use a semaphore object that has not been created; the functions
// always return in such cases
// TODO: Allow user process to create own semaphore object

#include "kernel_only.h"

SEMAPHORE sem[SEM_MAXNUMBER];	// the semaphore locks; maximum 256 of them 

/*** Initialize all semaphorees ***/
void init_semaphores() {
	int i;
	for (i=0; i<SEM_MAXNUMBER; i++) {
		init_queue(&(sem[i].waitq));
		sem[i].value = 0;
		sem[i].available = TRUE;
	}
	sem[0].available = FALSE;
}

/*** Create a semaphore object ***/
// At least one of the cooperating processes (typically
// the main process) should create the semaphore before use.
// The function returns 0 if no semaphore object is
// available; otherwise the semaphore object number is returned 
// init_value is the start value of the semaphore
sem_t semaphore_create(uint8_t init_value, PCB *p) {
	// TODO: see background material on what this function should do
	int i = 1;
	for(;i < SEM_MAXNUMBER; i++){
		if(sem[i].available == TRUE){
			sem[i].available == FALSE;
			sem[i].value = init_value;
			sem[i].creator = p->pid;
			//sem[i].waitq.head = 0;
			//sem[i].waitq.count = 0;
			while(sem[i].waitq.head != NULL){
				PCB *tempPCB = dequeue(&sem[i].waitq);
			}
			return i;
		}
	}

	// TODO: comment the following line before you start working
	return 0;
}

/*** Destroy a semaphore with a given key ***/
// This should be called by the process who created the semaphore
// using semaphore_create; the function makes the semaphore key available
// for use by other creators
// Semaphore is automatically destroyed if creator process dies; creator
// process should always destroy a semaphore when no other process is
// using it; otherwise the behavior is undefined
void semaphore_destroy(sem_t key, PCB *p) {
	// TODO: see background material on what this function should do
	if(sem[key].creator == p->pid){
		sem[key].available = TRUE;
		while(sem[key].waitq.head != NULL){
			PCB *tempPCB = dequeue(&sem[key].waitq);
			tempPCB->state = TERMINATED;
			sys_printf("A process was terminated because a semaphore was destroyed!\n");
		}
	}
}

/*** DOWN operation on a semaphore ***/
// Returns TRUE if process p is able to obtain semaphore
// number <key>; otherwise the process is queued and FALSE is
// returned.
bool semaphore_down(sem_t key, PCB *p) {
	// TODO: see background material on what this function should do
	if(sem[key].value == 0){
		p->state = WAITING;
		p->semaphore.wait_on = key;
		p->semaphore.queue_index = enqueue(&sem[key].waitq, p);
		return FALSE;
	}
	else{
		p->semaphore.wait_on = -1;
		sem[key].value--;
		return TRUE;
	}
	// TODO: comment the following line before you start working
	return TRUE;
}

/*** UP operation on a sempahore ***/
void semaphore_up(sem_t key, PCB *p) {
	// TODO: see background material on what this function should do
	sem[key].value++;
	if(sem[key].waitq.head != NULL){
		PCB *tempPCB = dequeue(&sem[key].waitq);
		tempPCB->state = READY;
		semaphore_down(key, tempPCB);
		/*
		if(semaphore_down(key, tempPCB))
		{
			tempPCB->state = READY;
		}
		*/

	}

}

/*** Cleanup semaphorees for a process ***/
void free_semaphores(PCB *p) {
	int i;

	for (i=1; i<SEM_MAXNUMBER; i++) {
		// see if process is creator of the semaphore
		if (p->pid == sem[i].creator) semaphore_destroy((sem_t)i,p);
	}

	// remove from wait queue, if any
	if (p->semaphore.wait_on != -1) 
		remove_queue_item(&sem[p->semaphore.wait_on].waitq, p->semaphore.queue_index);
	
}



