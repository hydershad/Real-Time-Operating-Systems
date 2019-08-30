// FIFO.h
// Runs on any LM3Sxxx
// Provide functions that initialize a FIFO, put data in, get data out,
// and return the current size.  The file includes a transmit FIFO
// using index implementation and a receive FIFO using pointer
// implementation.  Other index or pointer implementation FIFOs can be
// created using the macros supplied at the end of the file.
// Daniel Valvano
// June 16, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
      Programs 3.7, 3.8., 3.9 and 3.10 in Section 3.7

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#ifndef __FIFO_H__
#define __FIFO_H__ 1

#include "../../inc/tm4c123gh6pm.h"

long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value



// macro to create an index FIFO
#define AddIndexFifo(NAME,SIZE,TYPE,SUCCESS,FAIL) \
uint32_t volatile NAME ## PutI;    \
uint32_t volatile NAME ## GetI;    \
TYPE static NAME ## Fifo [SIZE];        \
void NAME ## Fifo_Init(void){ long sr;  \
  sr = StartCritical();                 \
  NAME ## PutI = NAME ## GetI = 0;      \
  EndCritical(sr);                      \
}                                       \
int NAME ## Fifo_Put (TYPE data){       \
  if(( NAME ## PutI - NAME ## GetI ) & ~(SIZE-1)){  \
    return(FAIL);      \
  }                    \
  NAME ## Fifo[ NAME ## PutI &(SIZE-1)] = data; \
  NAME ## PutI ## ++;  \
  return(SUCCESS);     \
}                      \
int NAME ## Fifo_Get (TYPE *datapt){  \
  if( NAME ## PutI == NAME ## GetI ){ \
    return(FAIL);      \
  }                    \
  *datapt = NAME ## Fifo[ NAME ## GetI &(SIZE-1)];  \
  NAME ## GetI ## ++;  \
  return(SUCCESS);     \
}                      \
unsigned short NAME ## Fifo_Size (void){  \
 return ((unsigned short)( NAME ## PutI - NAME ## GetI ));  \
}
// e.g.,
// AddIndexFifo(Tx,32,unsigned char, 1,0)
// SIZE must be a power of two
// creates TxFifo_Init() TxFifo_Get() and TxFifo_Put()

// macro to create a pointer FIFO
#define AddPointerFifo(NAME,SIZE,TYPE,SUCCESS,FAIL) \
TYPE volatile *NAME ## PutPt;    \
TYPE volatile *NAME ## GetPt;    \
TYPE static NAME ## Fifo [SIZE];        \
void NAME ## Fifo_Init(void){ long sr;  \
  sr = StartCritical();                 \
  NAME ## PutPt = NAME ## GetPt = &NAME ## Fifo[0]; \
  EndCritical(sr);                      \
}                                       \
int NAME ## Fifo_Put (TYPE data){       \
  TYPE volatile *nextPutPt;             \
  nextPutPt = NAME ## PutPt + 1;        \
  if(nextPutPt == &NAME ## Fifo[SIZE]){ \
    nextPutPt = &NAME ## Fifo[0];       \
  }                                     \
  if(nextPutPt == NAME ## GetPt ){      \
    return(FAIL);                       \
  }                                     \
  else{                                 \
    *( NAME ## PutPt ) = data;          \
    NAME ## PutPt = nextPutPt;          \
    return(SUCCESS);                    \
  }                                     \
}                                       \
int NAME ## Fifo_Get (TYPE *datapt){    \
  if( NAME ## PutPt == NAME ## GetPt ){ \
    return(FAIL);                       \
  }                                     \
  *datapt = *( NAME ## GetPt ## ++);    \
  if( NAME ## GetPt == &NAME ## Fifo[SIZE]){ \
    NAME ## GetPt = &NAME ## Fifo[0];   \
  }                                     \
  return(SUCCESS);                      \
}                                       \
unsigned short NAME ## Fifo_Size (void){\
  if( NAME ## PutPt < NAME ## GetPt ){  \
    return ((unsigned short)( NAME ## PutPt - NAME ## GetPt + (SIZE*sizeof(TYPE)))/sizeof(TYPE)); \
  }                                     \
  return ((unsigned short)( NAME ## PutPt - NAME ## GetPt )/sizeof(TYPE)); \
}
// e.g.,
// AddPointerFifo(Rx,32,unsigned char, 1,0)
// SIZE can be any size
// creates RxFifo_Init() RxFifo_Get() and RxFifo_Put()

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size);

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data);

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void);

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void);

#endif //  __FIFO_H__

