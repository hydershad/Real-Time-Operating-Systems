// OS.h
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include "../../inc/tm4c123gh6pm.h"
#include <stdint.h>

#ifndef __OS_H
#define __OS_H  1

#include "Semaphore.h"

// edit these depending on your clock        
#define TIME_1MS    80000
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/5)  

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void); 

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), 
   unsigned long stackSize, unsigned long priority);

//******** OS_AddProcess ***************
// Adds Process to the OS							(JUST A STUB FOR RIGHT NOW)
// Inputs: pointer to a void void process to add
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddProcess(void(*entry)(void), void *text, void *data, unsigned long stackSize, unsigned long priority);

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void);

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime); 

// ******** OS_Block ************
// place this thread into a blocked state
// input:  semaphore to be blocked on
// output: 1 if blocked, 0 if error
int OS_Block(Sema4Type* semaPt); 

// ******** OS_unBlock ************
// place this thread into a blocked state
// input:  semaphore to be blocked on
// output: 1 if unblocked, 0 if error
int OS_unBlock(Sema4Type* semaPt); 

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void); 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void);
 
//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice);

//******** Scheduler *************** 
//	A c function that allows us to determine what will be scheduled next
//  The systick handler calls this function in itself
//
void Scheduler(void);

//******** OS_DisTime *************** 
//	A c function that allows us to determine time disabled
void OS_DisTime(void);
//******** OS_EnTime *************** 
//	Another c function that allows us to determine time disabled
void OS_EnTime(void);

//******** ThreadProfiler *************** 
//	ThreadProfiler function
void ThreadProfiler(int16_t id);

#endif

