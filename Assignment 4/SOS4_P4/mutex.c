///////////////////////////////////////////////////////
// Mutex implementation
// This implementation provides MUTEX_MAXNUMBER mutex locks
// for use by user processes; the lock number is specified 
// using an 8-bit number (called key), which restricts us to 
// have up to 256 mutex variables
// DO NOT use a mutex object that has not been created; the functions
// always return in such cases
// TODO: Allow user process to create own mutex object

#include "kernel_only.h"

MUTEX mx[MUTEX_MAXNUMBER];	// the mutex locks; maximum 256 of them

/*** Initialize all mutex objects ***/
void init_mutexes() {
	int i;
	for (i=0; i<MUTEX_MAXNUMBER; i++) {
		mx[i].available = TRUE; // the mutex is available for use
		mx[i].lock_with = NULL;
		init_queue(&(mx[i].waitq));
	}
	mx[0].available = FALSE;
}

/*** Create a mutex object ***/
// At least one of the cooperating processes (typically
// the main process) should create the mutex before use
// The function returns 0 if no mutex objects are available;
// otherwise the mutex object number is returned
mutex_t mutex_create(PCB *p) {
	// TODO: see background material on what this function should do

	// TODO: comment the following line before you start working
	return 0;
}

/*** Destroy a mutex with a given key ***/
// This should be called by the process who created the mutex
// using mutex_create; the function makes the mutex key available
// for use by other creators.
// Mutex is automatically destroyed if creator process dies; creator
// process should always destroy a mutex when no other process is
// holding a lock on it; otherwise the behavior is undefined
void mutex_destroy(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do

}

/*** Obtain lock on mutex ***/
// Return true if process p is able to obtain a lock on mutex
// number <key>; otherwise the process is queued and FALSE is
// returned.
// Non-recursive: if the process holding the lock tries
// to obtain the lock again, it will cause a deadlock
bool mutex_lock(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do

	// TODO: comment the following line before you start working
	return TRUE;
}

/*** Release a previously obtained lock ***/
// Returns FALSE if lock is not owned by process p;
// otherwise the lock is given to a waiting process and TRUE
// is returned
bool mutex_unlock(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do

	// TODO: comment the following line before you start working
	return TRUE;
}

/*** Cleanup mutexes for a process ***/
void free_mutex_locks(PCB *p) {
	int i;

	for (i=1; i<MUTEX_MAXNUMBER; i++) {
		// see if process is creator of the mutex
		if (p->pid == mx[i].creator) mutex_destroy((mutex_t)i,p);
	}

	// remove from wait queue, if any
	if (p->mutex.wait_on != -1) 
		remove_queue_item(&mx[p->mutex.wait_on].waitq, p->mutex.queue_index);	
}



