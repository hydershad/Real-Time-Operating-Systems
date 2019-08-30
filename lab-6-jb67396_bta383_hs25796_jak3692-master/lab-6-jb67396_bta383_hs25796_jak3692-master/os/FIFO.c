// FIFO.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include "OS.h"
#include "FIFO.h"
#include "UART.h"
#include "Semaphore.h"

#define FIFOSIZE   512        // size of the FIFO (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure
                              // create index implementation FIFO (see FIFO.h)
uint32_t volatile OS_Fifo_PutI;
uint32_t volatile OS_Fifo_GetI; 
uint32_t static OS_Fifo [FIFOSIZE];  

// OS_FIFO semaphore
Sema4Type FifoAvailable;

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size)
{
	unsigned long sr = StartCritical();         
  OS_Fifo_PutI = OS_Fifo_GetI = 0;
	OS_InitSemaphore(&FifoAvailable, FIFOSIZE);
  EndCritical(sr);   
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data)
{										
  if((OS_Fifo_PutI - OS_Fifo_GetI ) & ~(FIFOSIZE-1)){  
    return(FIFOFAIL);      
  }
	OS_Signal(&FifoAvailable);
  OS_Fifo[OS_Fifo_PutI &(FIFOSIZE-1)] = data; 
  OS_Fifo_PutI++;  										
  return(FIFOSUCCESS);
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void)
{
  while( OS_Fifo_PutI == OS_Fifo_GetI ){ 
    //Spinning cooperatively
		OS_Wait(&FifoAvailable);
		//OS_Suspend();
  }                    
  uint32_t data = OS_Fifo[OS_Fifo_GetI & (FIFOSIZE-1)];  
  OS_Fifo_GetI++;  
  return data;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void)
{
	return OS_Fifo_PutI - OS_Fifo_GetI;
}

