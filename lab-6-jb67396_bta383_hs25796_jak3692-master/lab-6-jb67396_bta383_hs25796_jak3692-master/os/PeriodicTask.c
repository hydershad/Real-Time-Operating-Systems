// PeriodicTask.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include "os_inc/OS.h"
#include "os_inc/PeriodicTask.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void (*PeriodicTask1)(void);   // user function 1
void (*PeriodicTask2)(void);   // user function 2

// ***************** Timer1A_Init ****************
// Activate TIMER1 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq), 32 bits
// Outputs: none
void Timer1A_Init(void(*task)(void), uint32_t period, uint32_t priority)
{
	long sr;
  sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  PeriodicTask1 = task;          // user function
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period-1;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
	
	NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF1FFF)|(priority << 13); // 8) priority set from parameter
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
  EndCritical(sr);
}

void Timer1A_Handler(void)
{
	(*PeriodicTask1)();          // execute user task
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
}

// ***************** Timer3A_Init ****************
// Activate TIMER3 interrupts to set system time and
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq), 32 bits
// Outputs: none
void Timer3A_Init(void(*task)(void), unsigned long period, unsigned long priority)
{
	long sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
	PeriodicTask2 = task;
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    	// 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
	// THIS NEEDS TO CHANGE
  NVIC_PRI5_R = (NVIC_PRI8_R&0x1FFFFFFF)|(priority << 29); // 8) priority set from parameter
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN1_R = 1<<(35-32);          // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
  EndCritical(sr);
}

void Timer3A_Handler(void)
{
	(*PeriodicTask2)();
	TIMER3_ICR_R = TIMER_ICR_TATOCINT;  // acknowledge timeout
}

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
int OS_AddPeriodicThread(void(*task)(void), unsigned long period, unsigned long priority)
{
	if (PeriodicTask1 != 0 && PeriodicTask2 != 0)
	{
		// Already have 2 periodic tasks
		// Return failed
		return 0;
	}
	else if (PeriodicTask1 == 0) 
	{
		// Init the first task
		Timer1A_Init(task, period, priority);
	}
	else
	{
		// Init the second task
		Timer3A_Init(task, period, priority);
	}	
	return 1;
}

