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
	int i;
	int ret=0;
	for(i=0;i<MUTEX_MAXNUMBER;i++)
	{
		if(mx[i].available==TRUE)
		{
			ret=i;
			mx[i].available=FALSE;
			mx[i].creator=p->pid;
			//NULL?
			mx[i].lock_with=NULL;
			//reset waiting Q
			//mx[i].waitq.head=0;
			//mx[i].waitq.count=0;
			while(mx[i].waitq.head != NULL){
				PCB *tempPCB = dequeue(&mx[i].waitq);
			}

			return i;
		}
	}
	return 0;
	// TODO: comment the following line before you start working
}

/*** Destroy a mutex with a given key ***/
// This should be called by the process who created the mutex
// using mutex_create; the function makes the mutex key available
// for use by other creators.
// Mutex is automatically destroyed if creator process dies; creator
// process should always destroy a mutex when no other process is
// holding a lock on it; otherwise the behavior is undefined

//If there is something in the queue then make it p->state = TERMINATED
void mutex_destroy(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do

		if(mx[key].creator==p->pid)
		{
			mx[key].available=TRUE;
			while(mx[key].waitq.head != NULL)
			{
				PCB *tempPCB = dequeue(&mx[key].waitq);
				tempPCB->state = TERMINATED;
			//	sys_printf("A process was terminated because a mutex was destroyed!\n");
			}
		}
}

/*** Obtain lock on mutex ***/
// Return true if process p is able to obtain a lock on mutex
// number <key>; otherwise the process is queued and FALSE is
// returned.
// Non-recursive: if the process holding the lock tries
// to obtain the lock again, it will cause a deadlock
bool mutex_lock(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do
	/*
	 if the lock is with no process, give it to the calling process
	  (change lock_with) and return TRUE; 
	  otherwise, insert the calling process into the waiting queue of the mutex, and return FALSE.
	*/
	  if(mx[key].lock_with==NULL)
		{
			p->mutex.wait_on=-1;
			mx[key].lock_with=p;
			//add current to wait q
	//	sys_printf("lock is true\n");
			return TRUE;
		}
		else
		{
		
			p->state = WAITING;
			p->mutex.wait_on=key;
			p->mutex.queue_index=enqueue(&mx[key].waitq,p);
		//	sys_printf("lock is false\n");

			return FALSE;
		}

		/*
	If a process is going to be inserted into the queue of a mutex,
	 the mutex.wait_on and mutex.queue_index members of the process’s PCB are also updated. wait_on is set to the key of the mutex on which the process is going to wait,
	  and queue_index is the index in the waiting queue where this process is inserted. wait_on should be -1 for a process not in the waiting queue of a mutex.
	   These two members are used for cleanup if a process waiting in a mutex queue abnormally terminates (see free_mutex_locks).
		*/

	// TODO: comment the following line before you start working
	//return TRUE;
}

/*** Release a previously obtained lock ***/
// Returns FALSE if lock is not owned by process p;
// otherwise the lock is given to a waiting process and TRUE
// is returned
bool mutex_unlock(mutex_t key, PCB *p) {
	// TODO: see background material on what this function should do
	/*
	An unlock operation is not successful if the calling process does not own the lock on the mutex. 
	After a mutex is unlocked (lock_with=NULL),
	mutex_unlock gives the lock to the process at the head of the waiting queue, if any. 
	Accordingly, we will have to set lock_with, mutex.wait_on in the process PCB, and wake up the process.
	*/

	if(mx[key].lock_with==p)
	{
	
		mx[key].lock_with=NULL;
		//mx[key].available=TRUE;
		PCB *temp=dequeue(&mx[key].waitq);
		temp->state =READY;
		mutex_lock(key,temp);
	//	sys_printf("unlock is true\n");
		return TRUE;
		/*
		if(mutex_lock(key,temp))
		{
			temp->state=READY;

		}
		*/
	}

	// TODO: comment the following line before you start working
	//sys_printf("unlock is false\n");

	return FALSE;
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



