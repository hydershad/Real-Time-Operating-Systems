// Time.h
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#ifndef __TIME_H__ // do not include more than once
#define __TIME_H__

#include <stdint.h>

// ***************** Timer2A_Init ****************
// Activate Timer1A interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq), 32 bits
// Outputs: none
void Timer2A_Init(void(*task)(void));

// ***************** Timer2A_Clear ****************
// Resets the timer to 0
// Inputs:  none
// Outputs: none
void Timer2A_Clear(void);

// ***************** Timer2A_Clear ****************
// Resets the timer to 0
// Inputs:  none
// Outputs: none
void Timer2A_FakeClear(void);

// ***************** Timer2A_Clear ****************
// Resets the timer to 0
// Inputs:  none
// Outputs: none
uint32_t Timer2A_FakeCurrent(void);

// ***************** Timer2A_Clear ****************
// Resets the timer to 0
// Inputs:  none
// Outputs: none
uint32_t Timer2A_Current(void);

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void);

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop);

// ******** OS_ClearMsTime ************
// sets the system time to zero
// Inputs:  none
// Outputs: none
void OS_ClearMsTime(void);

// ******** OS_MsTime ************
// reads the current time in msec
// Inputs:  none
// Outputs: time in us units
unsigned long OS_MsTime(void);

#endif // __TIME_H__
