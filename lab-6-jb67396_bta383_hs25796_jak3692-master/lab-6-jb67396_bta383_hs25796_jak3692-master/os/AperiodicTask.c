// AperiodicTask.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen  & John Ballock
// March 28, 2019

#include "os_inc/AperiodicTask.h"
#include "../inc/tm4c123gh6pm.h"
#include "os_inc/Semaphore.h"
#include "os_inc/OS.h"

//******** Switch1_Handler *************** 
// A switch debouncing task
// This function is signaled by PortF_Handler
// Inputs: none
// Outputs: none
Sema4Type SW1_Pressed;			// semaphore for PortF_Handler 
uint8_t last1;							// the last switch1 input state
void(*UserSWTask1)(void);		// the task to run on press
void Switch1_Handler(void)
{
	last1 = GPIO_PORTF_DATA_R & 0x10;
	while(1)
	{
		// Wait for the handler to signal a touch/release
		OS_bWait(&SW1_Pressed);
		if(last1) 
		{
			// last1 was high so this is the press
			UserSWTask1();
		}
		// else 
		// {
		// 	// Put an on-release function here
		// }
		// Wait for bouncing to pass
		OS_Sleep(70);
		// Update last1 to be low
		last1 = GPIO_PORTF_DATA_R &=0x10;
		// Re-enable interrupt that the handler disabled
		GPIO_PORTF_IM_R |= 0x10;
		GPIO_PORTF_ICR_R = 0x10;
	}
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), unsigned long priority)
{
	// Initialize PF4 as active low
	SYSCTL_RCGCGPIO_R |= 0x20;					// activate clock for Port F
	OS_InitSemaphore(&SW1_Pressed, 0);
	GPIO_PORTF_LOCK_R = 0x4C4F434B;			// unloock GPIO on Port F
	GPIO_PORTF_CR_R = 0x1F;							// allow changes to PF4-0
	GPIO_PORTF_DIR_R &= ~0x10;						// make PF4 in
	GPIO_PORTF_DEN_R |= 0x10;						// enable digital IO on PF4
	GPIO_PORTF_PUR_R |= 0x10;						// Pull ups on PF4
	GPIO_PORTF_IS_R &= ~0x10;						// edge sensitive
	GPIO_PORTF_IBE_R |= 0x10;					  // both edges
	GPIO_PORTF_ICR_R = 0x10;						// clear flags
	GPIO_PORTF_IM_R |= 0x10;						// arm interrupt on PF4
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF) | (priority << 21);	// set priotiry
	NVIC_EN0_R = 0x40000000;						// enable interrupt 30 in NVIC

	// Assign the task
	UserSWTask1 = task;

	// Add thread that calls task1 
	OS_AddThread(Switch1_Handler, 100, priority);
	return 1;
}

//******** Switch2_Handler *************** 
// A switch debouncing task
// This function is signaled by PortF_Handler
// Inputs: none
// Outputs: none
Sema4Type SW2_Pressed;				// semaphore PortF_handler uses
uint8_t last2;								// last switch2 state
void(*UserSWTask2)(void);			// user function to run on press
void Switch2_Handler(void)
{
	last2 = GPIO_PORTF_DATA_R & 0x01;
	while(1)
	{
		// Wait for the handler to signal a touch/release
		OS_bWait(&SW2_Pressed);
		if(last2) 
		{
			// last2 was high so this is the press
			UserSWTask2();
		}
		// else 
		// {
		// 	// Put an on-release function here
		// }
		// Wait for bouncing to pass
		OS_Sleep(70);
		// Update last2 to be low
		last2 = GPIO_PORTF_DATA_R &=0x01;
		// Re-enable interrupt that the handler disabled
		GPIO_PORTF_IM_R |= 0x01;
		GPIO_PORTF_ICR_R = 0x01;
	}
}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority)
{
	// Initialize PF0 as active low
	SYSCTL_RCGCGPIO_R |= 0x20;					// activate clock for Port F
	OS_InitSemaphore(&SW2_Pressed, 0);
	GPIO_PORTF_LOCK_R = 0x4C4F434B;			// unloock GPIO on Port F
	GPIO_PORTF_CR_R = 0x1F;							// allow changes to PF4-0
	GPIO_PORTF_DIR_R &= ~0x01;					// make PF0 in
	GPIO_PORTF_DEN_R |= 0x01;						// enable digital IO on 0
	GPIO_PORTF_PUR_R |= 0x01;						// Pull ups on PF0
	GPIO_PORTF_IS_R &= ~0x01;						// edge sensitive
	GPIO_PORTF_IBE_R &= ~0x01;					// not both edges
	GPIO_PORTF_IEV_R |= 0x01;						// rising edge
	GPIO_PORTF_ICR_R = 0x11;						// clear flags
	GPIO_PORTF_IM_R |= 0x11;						// arm interrupt on PF4, 0
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF) | (priority << 21);	// priotiry 5
	NVIC_EN0_R = 0x40000000;						// enable interrupt 30 in NVIC

	// Assign the task
	UserSWTask2 = task;

	// Add the thread that calls task2 
	OS_AddThread(Switch2_Handler, 100, priority);
	return 1;
}

//******** GPIO_PortF_Handler *************** 
// Recieves the switch interrupts
// Signals for the user task to run
// Inputs: none
// Outputs: none
void GPIOPortF_Handler(void)
{
	if(GPIO_PORTF_RIS_R&0x10)
	{
		// Switch1 was pressed
		// Clear interrupt flag
		GPIO_PORTF_ICR_R = 0x10;
		// Signal to run task
		OS_bSignal(&SW1_Pressed);
		// Call user func
		//UserSWTask1();
		// Disable interrupts to prevent bouncing
		GPIO_PORTF_IM_R &= ~0x10;		
		// Interrupts are re-enabled in SW1 handler after sleep
	}
	if(GPIO_PORTF_RIS_R&0x01)
	{
		// Switch2 was pressed
		// Clear interrupt flag
		GPIO_PORTF_ICR_R = 0x01;
		// Signal to run task
		OS_bSignal(&SW2_Pressed);
		// Call user func
		//UserSWTask1();
		// Disable interrupts to prevent bouncing
		GPIO_PORTF_IM_R &= ~0x01;
		// Interrupts are re-enabled in SW2 handler after sleep
	}
	// Immediately call context switch
	OS_Suspend();
}


