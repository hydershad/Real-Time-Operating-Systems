// Time.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include <stdint.h>
#include "os_inc/Time.h"
#include "os_inc/OS.h"
#include "../inc/tm4c123gh6pm.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// Global for system time
volatile unsigned long sysTime;				// resolution in us
volatile unsigned long fakeTime;				// resolution in us
void (*PeriodicTask)(void);   // user function

// ***************** Timer2A_Init ****************
// Activate TIMER2 interrupts to set system time and
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq), 32 bits
// Outputs: none
void Timer2A_Init(void(*task)(void))
{
	uint32_t priority = 2;
	long sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate TIMER2
	PeriodicTask = task;
  TIMER2_CTL_R = 0x00000000;    // 1) disable TIMER2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = 80000-1;    	// 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear TIMER2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x1FFFFFFF)|(priority << 29); // 8) priority set from parameter
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 21 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable TIMER2A
  EndCritical(sr);
}

void Timer2A_Clear(void)
{
	TIMER2_TAR_R = 0;
	sysTime = 0;
}
void Timer2A_FakeClear(void)
{
	fakeTime = 0;
}
uint32_t Timer2A_FakeCurrent(void)
{
	return fakeTime;
}
uint32_t Timer2A_Current(void)
{
	// convert ms resolution sysTime to us resolution
	// add in the time from counter 
	// make this an upcounter for efficiency
	// CAN RUN FINE FOR ~72 minutes..........
	uint32_t usTime = (sysTime*1000) + ((80000 - TIMER2_TAR_R) / 80);			// Divides by 80 to use 1us timing
	// (TIMER2_TAILR_R - TIMER2_TAV_R)
	return usTime;
}

void Timer2A_Handler(void)
{
	ThreadProfiler(-1);
  TIMER2_ICR_R = TIMER_ICR_TATOCINT;
	sysTime++;
	fakeTime++;
	PeriodicTask();
	ThreadProfiler(-1);
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void)
{
	return Timer2A_Current();			// Return the us time
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop)
{
	return stop - start;					// Return the difference in us
}

// ******** OS_ClearMsTime ************
// sets the system time to zero
// Inputs:  none
// Outputs: none
void OS_ClearMsTime(void)
{
	Timer2A_FakeClear();
}

// ******** OS_MsTime ************
// reads the current time in msec
// Inputs:  none
// Outputs: time in us units
unsigned long OS_MsTime(void)
{
	return Timer2A_FakeCurrent();				// returns in us resolution
}

