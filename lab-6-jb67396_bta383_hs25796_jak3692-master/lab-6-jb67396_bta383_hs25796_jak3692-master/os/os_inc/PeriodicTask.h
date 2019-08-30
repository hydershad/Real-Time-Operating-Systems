// PeriodicTask.h
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#ifndef __PERIODICTASK_H__
#define __PERIODICTASK_H__ 1

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
int OS_AddPeriodicThread(void(*task)(void), unsigned long period, unsigned long priority);

#endif

