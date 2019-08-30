// FIFO.c
// Runs on any Cortex microcontroller
// Provide functions that initialize a FIFO, put data in, get data out,
// and return the current size.  The file includes a transmit FIFO
// using index implementation and a receive FIFO using pointer
// implementation.  Other index or pointer implementation FIFOs can be
// created using the macros supplied at the end of the file.
// Daniel Valvano
// May 2, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
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

#include <stdint.h>
#include "FIFO.h"
#include "OS.h"

#define FIFOSIZE 32
#define FIFOSUCCESS 1
#define FIFOFAIL 0

int fsize = 0;
DataType volatile *PutPt;
DataType volatile *GetPt;
extern Sema4Type RoomLeft;
extern Sema4Type mutex;
extern Sema4Type CurrentSize;

DataType static Fifo[FIFOSIZE];
//initialize fifo
int getFifoSize(void){
	return fsize;
}
	

void Fifo_Init(void){
	
	PutPt = GetPt = &Fifo[0]; //empty fifo
	OS_InitSemaphore(&RoomLeft, FIFOSIZE -1);
	OS_InitSemaphore(&mutex, 1);
	OS_InitSemaphore(&CurrentSize, 0);
		
}

unsigned long Fifo_Put(DataType data){
	//OS_Wait(&mutex);//COME BACK
		DataType volatile *nextPutPt;
	nextPutPt = PutPt+1;
	if(nextPutPt == &Fifo[FIFOSIZE]){
			nextPutPt = &Fifo[0];
	}
	if(nextPutPt == GetPt){return (FIFOFAIL);}
	else{
			*(PutPt) = data;
		PutPt = nextPutPt;
		OS_Signal(&CurrentSize);
		return (FIFOSUCCESS);
	}
	/*
OS_Wait(&RoomLeft);
OS_Wait(&mutex);
*(PutPt++) = data;
if(PutPt == &Fifo[FIFOSIZE]){
	PutPt = &Fifo[0];
}	
OS_Signal(&mutex);
OS_Signal(&CurrentSize);
*/
}


unsigned long Fifo_Get(DataType *datapt){
OS_Wait(&CurrentSize);
	OS_Wait(&mutex);
	if(PutPt == GetPt){return FIFOFAIL;}
	*datapt = *(GetPt++);
	if(GetPt == &Fifo[FIFOSIZE]){
			GetPt = &Fifo[0];
	}
	OS_Signal(&mutex);
	/*
	OS_Wait(&CurrentSize);
	OS_Wait(&mutex);
	*datapt = *(GetPt++);
	if(GetPt == &Fifo[FIFOSIZE]){
		GetPt = &Fifo[0];
	}
	OS_Signal(&mutex);
	OS_Signal(&RoomLeft);
	*/
	/*
	if(PutPt == GetPt){ return FIFOFAIL;}
	*datapt = *(GetPt++);
	if(GetPt == &Fifo[FIFOSIZE]){ GetPt = &Fifo[0];}
	fsize = fsize - 1;
	return FIFOSUCCESS;
	*/
return 1;
}
