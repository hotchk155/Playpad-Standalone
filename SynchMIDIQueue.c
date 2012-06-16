/////////////////////////////////////////////////////////////////////////////
// Synchronised MIDI Queue
/////////////////////////////////////////////////////////////////////////////
#include "stdlib.h"
#include "vos.h"
#include "SynchMIDIQueue.h"

#define SMQ_LEN 50
SMQ_MSG queue[SMQ_LEN];
int head; 
int tail;
vos_semaphore_t semAvailable;
vos_mutex_t mAccess;

/////////////////////////////////////////////////////////////////////////////
// INIT
void SMQInit()
{
	head = 0;
	tail = 0;
	vos_init_semaphore(&semAvailable,0);
	vos_init_mutex(&mAccess,0);	
}
	
/////////////////////////////////////////////////////////////////////////////
// WRITE
void SMQWrite(SMQ_MSG *msg)
{
	int newHead;
	vos_lock_mutex(&mAccess);
	newHead = head + 1;
	if(newHead >= SMQ_LEN)
		newHead = 0;
	if(newHead != tail)
	{
		queue[head].status = msg->status;
		queue[head].param1 = msg->param1;
		queue[head].param2 = msg->param2;
		head = newHead;		
		vos_signal_semaphore(&semAvailable);		
	}
	vos_unlock_mutex(&mAccess);
}
	
/////////////////////////////////////////////////////////////////////////////
// READ
void SMQRead(SMQ_MSG *msg)
{
	vos_wait_semaphore(&semAvailable);
	vos_lock_mutex(&mAccess);
	if(head == tail) 
	{
		vos_unlock_mutex(&mAccess);
	}
	else
	{
		msg->status = queue[tail].status;		
		msg->param1 = queue[tail].param1;		
		msg->param2 = queue[tail].param2;		
		if(++tail >= SMQ_LEN)
			tail = 0;
		vos_unlock_mutex(&mAccess);
	}
}
	
/////////////////////////////////////////////////////////////////////////////
// WAITING
int SMQWaiting()
{
	int result = 0;
	vos_lock_mutex(&mAccess);
	if(head != tail) 
		result = 1;
	vos_unlock_mutex(&mAccess);
	return result;
}
