// Semaphore.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include "os_inc/Semaphore.h"
#include "os_inc/OS.h"

long StartCritical(void); // Disable interrupts
void EndCritical(long);  // Enable interrupts
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts


// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value){
	semaPt->Value = value;
}

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
	long status = StartCritical();
	semaPt->Value = semaPt->Value - 1;
	if (semaPt->Value < 0)
	{
		// enter a blocked status
		OS_Block(semaPt);
		
		// suspend
		OS_Suspend();
	}
	EndCritical(status);
}

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
	long status = StartCritical();
	semaPt->Value = semaPt->Value + 1;
	if (semaPt->Value <= 0)
	{
		// leave the blocked status
		OS_unBlock(semaPt);
		//OS_Suspend();
	}
	EndCritical(status);
}

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
	OS_DisableInterrupts();
	// semaphore not available
	if (semaPt->Value == 0)
	{
		// enter a blocked status
		OS_Block(semaPt);
		
		// suspend
		OS_Suspend();
	}
	// else retrieve semaphore
	else
	{
		semaPt->Value = 0;
	}
	OS_EnableInterrupts();
}

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	long status = StartCritical();
	// Threads are waiting
	if (semaPt->Value == 0)
	{
		// leave the blocked status
		// set sema value if no threads are blocked
		if(!OS_unBlock(semaPt)) semaPt->Value = 1;
	}
	EndCritical(status);
}

