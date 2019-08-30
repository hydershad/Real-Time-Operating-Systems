//OS.c

// filename **********OS.H***********
// Real Time Operating System for Labs 2 and 3 
// Jonathan W. Valvano 2/20/17, valvano@mail.utexas.edu
// EE445M/EE380L.12
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file
// You are required to implement the spirit of this OS
#include "OS.h"
#include "tm4c123gh6pm.h"
#include "Timer2.h"
#include "FIFO.h"
#include "PLL.h"

#define STACKSIZE 120
#define NUMBEROFTHREADS 50
#define NUMTHREADS 8
#define NUMPRIORITIES 7
#define PE0  (*((volatile unsigned long *)0x40024004))
	

uint32_t count = 0;
Sema4Type RoomLeft;
Sema4Type mutex;
Sema4Type CurrentSize;
unsigned long maxInterruptTime;
unsigned long disabled, enabled;

	unsigned long MailBox = 0;		//data to communicate
	struct Sema4 DataValid;						//new mail or already read mail, 0 = empty mailbox
	struct Sema4 BoxFree;	
uint32_t OSTimerCount = 0;
long StartCritical(void);
void DisableInterrupts(void);
void EnableInterrupts(void);
void EndCritical(long primask);
tcbType dummy;
struct sleepList sleepingThreads;

// feel free to change the type of semaphore, there are lots of good solutions

tcbType tcbs[NUMPRIORITIES][NUMTHREADS];
tcbType *RunPt;
tcbType* priorityPts[NUMPRIORITIES];
tcbType *SleepPt = 0;
Sema4Type LCDFree;
long Stacks[NUMPRIORITIES][NUMTHREADS][STACKSIZE];


void OS_DisableInterrupts(void){	
	DisableInterrupts();
	PE0 = 1;
	disabled = OS_Time();
}

void OS_EnableInterrupts(void){
	EnableInterrupts();
	PE0 = 0;
	enabled = OS_Time();
	if((enabled - disabled) > maxInterruptTime){
		maxInterruptTime = enabled - disabled;
	}
}

void SetInitialStack(int i, unsigned long priority){

	tcbs[priority][i].sp = &Stacks[priority][i][STACKSIZE-16]; // thread stack pointer
  Stacks[priority][i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[priority][i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[priority][i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[priority][i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[priority][i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[priority][i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[priority][i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[priority][i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[priority][i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[priority][i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[priority][i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[priority][i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[priority][i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[priority][i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[priority][i][STACKSIZE-16] = 0x04040404;  // R4
}

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void){
  DisableInterrupts();
  PLL_Init(Bus80MHz);         // set processor clock to 50 MHz
	SYSCTL_RCGCTIMER_R |= 0x20;      // 0) activate timer5
  TIMER5_CTL_R &= ~0x00000001;     // 1) disable timer5A during setup
  TIMER5_CFG_R = 0x00000004;       // 2) configure for 16-bit timer mode
  TIMER5_TAMR_R = 0x00000002;      // 3) configure for periodic mode, default down-count settings
  TIMER5_TAILR_R = 7999;       // 4) reload value
  TIMER5_TAPR_R = 0;              // 5) 1us timer5A
  TIMER5_ICR_R = 0x00000001;       // 6) clear timer5A timeout flag
  TIMER5_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
  NVIC_PRI23_R = (NVIC_PRI23_R&0xFFFFFF00)|0x00000020; // 8) priority 1
  NVIC_EN2_R |= 0x10000000;        // 9) enable interrupt 19 in NVIC
  // vector number 108, interrupt number 92
  //TIMER5_CTL_R |= 0x00000001;      // 10) enable timer5A
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value){
	(*semaPt).Value = value;
	(*semaPt).next = 0;
	(*semaPt).occupants = 0;
	
}

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
	long status;
	status = StartCritical();
	OS_DisableInterrupts();
	if(semaPt->Value > 0){		//sema4 is not being used by any other thread
		semaPt->Value--;
		EndCritical(status);
		OS_EnableInterrupts();
		return;
	}
	
		//this will always execute if sema4 is currently being used by another thread
	if((RunPt->parent) != (RunPt)){		//if it is NOT the only thread in the priority
		RunPt->parent->next = RunPt->next;
		RunPt->next->parent = RunPt->parent;
		if(priorityPts[RunPt->priority] == RunPt){
		priorityPts[RunPt->priority] = RunPt->next;
		}
	}
	else{	//only this thread exists in priority queue
		priorityPts[RunPt->priority] = 0;
	}
		RunPt->semaphore = 1;		
		semaPt->Value--;
	
	//if the sema4 is not available
	if(semaPt->occupants == 0){
		semaPt->next = RunPt;
		//RunPt->sema4Link = &dummy;
		RunPt->sema4Link = 0;
		RunPt->parent = 0;
		semaPt->occupants++;
		
		EndCritical(status);
		OS_EnableInterrupts();
		OS_Suspend();
		return;		
	}
	
	
	

	tcbType* temp = semaPt->next;
	for(int index = semaPt->occupants; index > 0; index--){
			if(RunPt->priority > temp->priority){		//new thread has higher priority than thread next in queue
					if(temp->parent == 0){		//if it is the first in the list
							RunPt->sema4Link = semaPt->next;
							semaPt->next->parent = RunPt;
							//semaPt->next->sema4Link = &dummy;
						if(semaPt->occupants == 1){
							semaPt->next->sema4Link = 0;
						}
							semaPt->next = RunPt;
							RunPt->parent = 0;
							semaPt->occupants++;
							break;
					}
					
					//not the first in list
					RunPt->sema4Link = temp;
					RunPt->parent = temp->parent;
					temp->parent->sema4Link = RunPt;
					temp->parent = RunPt;
					semaPt->occupants++;
					break;
			}
			if(temp->sema4Link == 0){
				temp->sema4Link = RunPt;
				RunPt->parent = temp;
				RunPt->sema4Link = 0;
				semaPt->occupants++;
				break;
			
			}
		
		temp = temp->sema4Link;
	}		
	EndCritical(status);
	OS_EnableInterrupts();
	OS_Suspend();
	return;
}
// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
	long status;
	status = StartCritical();
OS_DisableInterrupts();
	semaPt->Value++;
	
	
	//there is nothing in the list
	if(semaPt->Value > 0){	// TODO: come back
		//semaPt->next = 0;
		EndCritical(status);
		OS_EnableInterrupts();
		return;
	}
	
	
	//there is something in the list
	semaPt->occupants--;
	semaPt->next->semaphore = 0;
	int priority = semaPt->next->priority;
	
	if(priorityPts[priority] != 0){
	semaPt->next->parent = priorityPts[priority]->parent;		//top of smae4 list is added to end of priority list
	semaPt->next->next = priorityPts[priority];
	semaPt->next->parent->next = semaPt->next;
	semaPt->next->next->parent = semaPt->next;
	}
	else{
		priorityPts[priority] = semaPt->next;
		semaPt->next->next = semaPt->next;
		semaPt->next->parent = semaPt->next;
	}
	
	
	//there is something list
	if(semaPt->occupants > 0){
	semaPt->next->sema4Link->parent = 0;
	
	semaPt->next = semaPt->next->sema4Link;
	}
	else{
		semaPt->next = 0;
	}
	
	EndCritical(status);
	OS_EnableInterrupts();
	return;
}	

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
	long status;
	status = StartCritical();
	OS_DisableInterrupts();
	if(semaPt->Value > 0){		//sema4 is not being used by any other thread
		semaPt->Value = 0;
		EndCritical(status);
		OS_EnableInterrupts();
		return;
	}
	
		//this will always execute if sema4 is currently being used by another thread
	if((RunPt->parent) != (RunPt)){		//if it is NOT the only thread in the priority
		RunPt->parent->next = RunPt->next;
		RunPt->next->parent = RunPt->parent;
		if(priorityPts[RunPt->priority] == RunPt){
		priorityPts[RunPt->priority] = RunPt->next;
		}
	}
	else{	//only this thread exists in priority queue
		priorityPts[RunPt->priority] = 0;
	}
		RunPt->semaphore = 1;		
		//semaPt->Value = 0;
	
	//if the sema4 is not available
	if(semaPt->occupants == 0){
		semaPt->next = RunPt;
		RunPt->sema4Link = 0;
		RunPt->parent = 0;
		semaPt->occupants++;
		
		EndCritical(status);
		OS_EnableInterrupts();
		OS_Suspend();
		return;		
	}
	
	
	

	tcbType* temp = semaPt->next;
	for(int index = semaPt->occupants; index > 0; index--){
			if(RunPt->priority > temp->priority){		//new thread has higher priority than thread next in queue
					//if(temp->parent == 0){		//if it is the first in the list
				if(temp == semaPt->next){	//first in the list		
							RunPt->sema4Link = semaPt->next;
							semaPt->next->parent = RunPt;
						if(semaPt->occupants == 1){
							semaPt->next->sema4Link = 0;
						}
							//semaPt->next->sema4Link = &dummy;
							semaPt->next = RunPt;
							RunPt->parent = 0;
							semaPt->occupants++;
							break;
					}
					
					//not the first in list
					RunPt->sema4Link = temp;
					RunPt->parent = temp->parent;
					temp->parent->sema4Link = RunPt;
					temp->parent = RunPt;
					semaPt->occupants++;
					break;
			}
			if(temp->sema4Link == 0){
				temp->sema4Link = RunPt;
				RunPt->parent = temp;
				RunPt->sema4Link = 0;
				semaPt->occupants++;
				break;
			
			}
		
		temp = temp->sema4Link;
	}		
	EndCritical(status);
	OS_EnableInterrupts();
	OS_Suspend();
	return;

}	

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	long status;
	status = StartCritical();
	OS_DisableInterrupts();
	
	
	
	//there is nothing in the list
	if(semaPt->occupants == 0){
		semaPt->Value = 1;
		semaPt->next = 0;
		EndCritical(status);
		OS_EnableInterrupts();
		return;
	}
	
	
	//there is something in the list
	semaPt->occupants--;
	semaPt->next->semaphore = 0;
	int priority = semaPt->next->priority;
	
	if(priorityPts[priority] != 0){
	semaPt->next->parent = priorityPts[priority]->parent;		//top of smae4 list is added to end of priority list
	semaPt->next->next = priorityPts[priority];
	semaPt->next->parent->next = semaPt->next;
	semaPt->next->next->parent = semaPt->next;
	}
	else{
		priorityPts[priority] = semaPt->next;
		semaPt->next->next = semaPt->next;
		semaPt->next->parent = semaPt->next;
	}
	
	
	//there is something list
	if(semaPt->occupants > 0){
	semaPt->next->sema4Link->parent = 0;
	
	semaPt->next = semaPt->next->sema4Link;
	}
	else{
		semaPt->next = 0;
	}
	EndCritical(status);
	OS_EnableInterrupts();
	return;
}	

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int threadCount = 1;
int priorityCount[NUMPRIORITIES];
int OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority){
	OS_DisableInterrupts();
long status;
status = StartCritical();
	int i;
for(i = 0; (i < NUMTHREADS) && (tcbs[priority][i].Identifier != 0); i++){
}
if(i == NUMTHREADS){		//not enough space in TCB array
	EndCritical(status);
	OS_EnableInterrupts();	
	return 0;
}
tcbs[priority][i].Identifier = (i + 1)+(priority*8);
tcbs[priority][i].priority = priority;
if(priorityPts[priority] != 0){
	tcbs[priority][i].parent = priorityPts[priority]->parent;
	tcbs[priority][i].next = priorityPts[priority];
	priorityPts[priority]->parent->next = &tcbs[priority][i];
	priorityPts[priority]->parent = &tcbs[priority][i];
	//int j;
//	for(j = NUMTHREADS-1; ((j>0) && (tcbs[priority][j].Identifier == 0)); j--){}
	//	tcbs[priority][0].parent = &tcbs[priority][j];
}
else{
		//int k;
	//for(k = NUMTHREADS-1; (k > 0)&&(tcbs[priority][k].Identifier == 0); k--){		//setting the tcb in the thread and reordering
	//}
	/*
	tcbs[priority][i].parent = &tcbs[priority][k];
	tcbs[priority][i].next = tcbs[priority][k].next;
	tcbs[priority][k].next->parent = &tcbs[priority][i];
	tcbs[priority][k].next = &tcbs[priority][i];
	*/
	priorityPts[priority] = &tcbs[priority][i];
	priorityPts[priority]->next = priorityPts[priority];
	priorityPts[priority]->parent = priorityPts[priority];
}
SetInitialStack(i, priority);
Stacks[priority][i][STACKSIZE - 2] = (long)(task);
	if(threadCount == 1){
	RunPt = &tcbs[priority][0];
	threadCount++;
	}
	//if((priorityCount[priority] == 0) || (priorityPts[priority] == 0)){
		//priorityPts[priority] = &tcbs[priority][i];
	//}
	priorityCount[priority]++;
	EndCritical(status);
	OS_EnableInterrupts();
return 1;
}


//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
	return RunPt->Identifier;
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
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void), 
   unsigned long period, unsigned long priority){
		 Timer2_Init((*task), period, priority);
		 return 1;
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
int OS_AddSW1Task(void(*task)(void), unsigned long priority){
		OS_DisableInterrupts();
		Switch_Init((*task));
		Timer3_Init(16000);
		OS_EnableInterrupts();
		return 1;
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
int OS_AddSW2Task(void(*task)(void), unsigned long priority){
		OS_DisableInterrupts();
		Switch_Init2((*task));
		Timer3_Init(16000);
		OS_EnableInterrupts();
		return 1;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime){
	OS_DisableInterrupts();
	RunPt->sleepNumber = sleepTime;
	RunPt->sema4Link = 0;
	
		if(priorityPts[RunPt->priority] == RunPt){	//if priority pointer is pointing at the running thread
		if(RunPt->next != RunPt){		//if there is more than one thread in that priority queue
				priorityPts[RunPt->priority] = RunPt->next;
				RunPt->next->parent = RunPt->parent;
				RunPt->parent->next = RunPt->next;
		}
		else{
			priorityPts[RunPt->priority] = 0;		//that is the only thread in that priority queue
		}
		
		
	}
	else{			//priority pointer is not pointing at the currently running thread meaning there is more than one thread in the priority queue
				RunPt->next->parent = RunPt->parent;
				RunPt->parent->next = RunPt->next;
	}
	
	
	
	if(sleepingThreads.occupants == 0){		//sleep list is empty
		sleepingThreads.start = RunPt;	//insert thread into list
		RunPt->parent = 0;
		RunPt->sema4Link = 0;
	}
	else{		//sleep list has items inside
		
		sleepingThreads.end->sema4Link = RunPt;	//place thread at end of list
		RunPt->parent = sleepingThreads.end;
		RunPt->sema4Link = 0;
	}
		sleepingThreads.end = RunPt;
	
	sleepingThreads.occupants++;
	OS_EnableInterrupts();
	OS_Suspend();
	
}	

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){		
	OS_DisableInterrupts();
	RunPt->parent->next = RunPt->next;
	RunPt->next->parent = RunPt->parent;
	//RunPt->parent = 0;
	RunPt->Identifier = 0;
	priorityCount[RunPt->priority]--;		//decrements the total number of threads in the scpecific priority list
	if(priorityPts[RunPt->priority] == RunPt){
		priorityPts[RunPt->priority] = RunPt->next;
	}
	if(RunPt->next == RunPt){		//if there is nothing left set the pointer to null
		priorityPts[RunPt->priority] = 0;
	}
	OS_EnableInterrupts();
	OS_Suspend();
}	

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
	NVIC_ST_CURRENT_R = 0;
	NVIC_INT_CTRL_R = 0x04000000;
	//NVIC_INT_CTRL_R = 0x10000000;
}
 
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size){
	Fifo_Init();
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data){
	int status = 0;
	status = Fifo_Put(data);
	return status;
}
// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){
unsigned long data = 0;
	int status = 0;
	status = Fifo_Get(&data);
	if(status){return data;}
	else{return 0;}
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void){
	return 0;
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
	 MailBox = 0;		//data to communicate
	 OS_InitSemaphore(&DataValid, 0);
	 OS_InitSemaphore(&BoxFree, 1);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data){
	OS_bWait(&BoxFree);
	MailBox = data;
	
	OS_bSignal(&DataValid);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void){
	int data = 0;
	OS_bWait(&DataValid);
	data = MailBox;
	OS_bSignal(&BoxFree);
	
	return data;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void){
	int temp = OSTimerCount;
	int other = TIMER5_TAR_R;
	other = TIMER5_TAILR_R - other;
	other =  other/8;
	//return (((temp) * (8000)) + other); //resolution of 12.5 nS
	return ((((temp) * (1000)) + other) / 10);	//resolution of 1 uS
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
return (stop - start);
}
// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){		//come back
	OSTimerCount = 0;
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void){
	int temp = OSTimerCount;
	int other = TIMER5_TAR_R;
	other = TIMER5_TAILR_R - other;
	other = other/8;
	return (((temp * 1000) + other) / 100000);
}

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(unsigned long theTimeSlice){
	//TIMER5_CTL_R &= ~0x00000001;
	OSTimerCount = 0;
  NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
	TIMER5_CTL_R |= 0x00000001;      // 10) enable timer5A
  StartOS();                   // start on the first task
}


void Timer5A_Handler(void){ // interrupts after each block is transferred
  TIMER5_ICR_R = TIMER_ICR_TATOCINT; // acknowledge timer5A timeout
	OS_DisableInterrupts();		//COME BACK might take too long to execute
  OSTimerCount++;
	count++;
	if(count == 10){		//if 1 milisecond has passed, we go into each priority list and decrement sleep values
		tcbType *temp = sleepingThreads.start;
		for(int i = sleepingThreads.occupants; i > 0; i--){
			temp->sleepNumber--;
			if(temp->sleepNumber == 0){		//if it is time to wake up
				
				if((temp != sleepingThreads.start) && (temp != sleepingThreads.end)){
					temp->parent->sema4Link = temp->sema4Link;
					temp->sema4Link->parent = temp->parent;
				}
				
				if(temp == sleepingThreads.start){	//if it is the start of the list
					sleepingThreads.start = temp->sema4Link;
					if(temp->sema4Link != 0){			   //if it is not the only thing in the list
					temp->sema4Link->parent = 0;
					}
				}
				if(temp == sleepingThreads.end){	//
					sleepingThreads.end = temp->parent;
					if(temp->parent != 0){
						temp->parent->sema4Link = 0;
					}
				}
				
				
				if(priorityPts[temp->priority] == 0){		//if there is nothing in the priority queue
					priorityPts[temp->priority] = temp;
					temp->next = temp;
					temp->parent = temp;
				}
				else{
					priorityPts[temp->priority]->parent->next = temp;		//last node in the list
					temp->next = priorityPts[temp->priority];		//first node in the list
					temp->parent = temp->next->parent;	//the old parent of the first node in the list
					temp->next->parent = temp;
				}
				
				sleepingThreads.occupants--;
			}
			temp = temp->sema4Link;
		}
		count = 0;
	}
	OS_EnableInterrupts();
}

