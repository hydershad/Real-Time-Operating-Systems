// MailBox.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include "os_inc/OS.h"
#include "os_inc/MailBox.h"
#include "os_inc/Semaphore.h"

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
Sema4Type boxFree;
Sema4Type dataValid;
unsigned long box;
void OS_MailBox_Init(void)
{
	OS_InitSemaphore(&boxFree, 1);
	OS_InitSemaphore(&dataValid, 0);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data)
{
	OS_bWait(&boxFree);
	box = data;
	OS_bSignal(&dataValid);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void)
{
	OS_bWait(&dataValid);
	unsigned long data = box;
	OS_bSignal(&boxFree);
	return data;
}


