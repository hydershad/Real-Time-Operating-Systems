// os.c
// Runs on LM4F120/TM4C123

#include <stdint.h>
#include "os.h"
#include "PLL.h"
#include "UART.h"
#include "FIFO.h"
#include "Semaphore.h"
#include "Time.h"
#include "ST7735.h"
#include "Heap.h"
#include "tm4c123gh6pm.h"

void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
void StartOS(void);
void OS_DecrementSleep(void);

// Thread constraints
#define STACKSIZE 128
#define NUMTHREADS 20
#define NUMPROC 5
#define NUMPRIORITIES 8
#define FIFOSIZE 128

// Debugging variables
unsigned long profile = 0;
unsigned long threadProfilerTime[100];
long threadProfilerId[100];

// Used to assign thread IDs
unsigned long globalID = 0;
unsigned long globalPID = 0;
unsigned long addedPID = 0;
	
// Process Control Block
struct PCB {
	void *text;				// read only the code segment
	void *data;				// read write the additional data
	int32_t procID;				// the process ID
	int32_t used;					// if this pcb is in use
	void(*entry)(void);		// entry function to the process
	int32_t numThreads;		// number of threads this process spawned
};	
typedef struct PCB PCBtype;
	
// Thread Control Block
struct TCB {
	int32_t *savedSP;			// the stack pointer for this thread
	int32_t used;					// if this tcb is in use
	int32_t ID;						// id of this thread
	int32_t PID;					// id of process this thread belongs to
	uint32_t priority;		// pri of this thread
	uint32_t stackSize;		// stack size for thread
	Sema4Type *blocked;		// blocked state of the thread
	int32_t sleep;				// sleep left on the thread
	struct TCB *last;			// prev thread in list
	struct TCB *next;			// next thread in list
};
typedef struct TCB TCBtype;

// Currently running thread
TCBtype *runPt;
// List of active threads
TCBtype tcbList[NUMTHREADS];
// List of active processes
PCBtype pcbList[NUMPROC];
// Stacks for each active thread
int32_t tcbStack[NUMTHREADS][STACKSIZE];
// Array of pointers to different priorities contained in tcbList
TCBtype *priM[NUMPRIORITIES];

int init = 0;

// ******** SetInitialStack ************
// Internal function used to set the initial state of the stack for a new thread 
// input:  tcbList index for the thread
// output: none
void SetInitialStack(int i){
  tcbList[i].savedSP = &tcbStack[i][STACKSIZE-16]; // thread stack pointer
  tcbStack[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  tcbStack[i][STACKSIZE-3] = 0x14141414;   // R14
  tcbStack[i][STACKSIZE-4] = 0x12121212;   // R12
  tcbStack[i][STACKSIZE-5] = 0x03030303;   // R3
  tcbStack[i][STACKSIZE-6] = 0x02020202;   // R2
  tcbStack[i][STACKSIZE-7] = 0x01010101;   // R1
  tcbStack[i][STACKSIZE-8] = 0x10000000;   // R0
  tcbStack[i][STACKSIZE-9] = 0x11111111;   // R11
  tcbStack[i][STACKSIZE-10] = 0x10101010;  // R10
  tcbStack[i][STACKSIZE-11] = (uint32_t) pcbList[tcbList[i].PID].data;  // R9
  tcbStack[i][STACKSIZE-12] = 0x08080808;  // R8
  tcbStack[i][STACKSIZE-13] = 0x07070707;  // R7
  tcbStack[i][STACKSIZE-14] = 0x06060606;  // R6
  tcbStack[i][STACKSIZE-15] = 0x05050505;  // R5
  tcbStack[i][STACKSIZE-16] = 0x04040404;  // R4
}

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none

void OS_Init(void)
{
	OS_DisableInterrupts();
	int i;
	// Initialize the pcbList
	for(i = 0; i < NUMPROC; i++){
		pcbList[i].procID = 0;
		pcbList[i].numThreads = 0;
		pcbList[i].used = 0;
		if(i == 0)
			pcbList[i].used = 1;
	}
	
	// Initialize the tcbList
	for(i = 0; i < NUMTHREADS; i++){
		tcbList[i].used = 0;
		tcbList[i].blocked = 0;
		tcbList[i].sleep = 0;
	}
	
	// Initialize the priority mask list
	for(i = 0; i < NUMPRIORITIES; i++){
		priM[i] = 0;
	}
	
	// Initialize the sysTick for context switching
	NVIC_ST_CTRL_R = 0;         																// disable SysTick during setup
	NVIC_ST_CURRENT_R = 0;      																// any write to current clears it
	NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; 	// priority 7
	
	// Init the system time timer used for OS_Time and OS_Sleep
	Timer2A_Init(&OS_DecrementSleep);
	
	init = 1;
}

//******** OS_AddThread ***************
// adds foregound thread to the scheduler
// Inputs: pointer to a void void thread to add
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddThread(void(*task0)(void), unsigned long stackSize, unsigned long priority){ 
	long status = StartCritical();
	
	// TODO use stackSize to dynamically allocate stack space 
	// (we will need to restructure if this is the case we have tcbStack already statically)
	// (MAYBE DONE) Increment number of threads in its process
	
	
	// Find the next open block in the tcbList
	int me = 0;
	while(tcbList[me].used){
		me++;
		if(me >= NUMTHREADS)
			return 0;
	}
	tcbList[me].used = 1;
	// Attempt at keeping track of threads in a process
	if(addedPID == 0){
		if (runPt == 0) {
			// If runPt == 0 then first time thread running
			// So PID = 0?
			tcbList[me].PID = 0;
			runPt = &tcbList[me];
		}
		else{
			tcbList[me].PID = runPt->PID;
		}
		int i;
		for(i = 0; i < NUMPROC; i++){
			if(tcbList[me].PID == pcbList[i].procID){
				pcbList[i].numThreads++;
				break;
			}
		}		
	}
	else{
		tcbList[me].PID = addedPID;
		addedPID = 0;
	}	
	
	
	
	// Init the stack and 
	SetInitialStack(me); 					 									
	tcbList[me].ID = globalID++;
	tcbList[me].blocked = 0;
	tcbList[me].sleep = 0;
	tcbList[me].priority = priority;
	// Set LR on stack
	tcbStack[me][STACKSIZE-2] = (int32_t)(task0);
	
	
	
	// New stuff
	if(priM[priority] == 0)
		priM[priority] = &tcbList[me];
	tcbList[me].last = priM[priority]->last;
	tcbList[me].next = priM[priority];
	priM[priority]->last = &tcbList[me];
	tcbList[me].last->next = &tcbList[me];
	
									 
  EndCritical(status);
  return 1;                                       // successful
}

//******** OS_AddProcess ***************
// Adds Process to the OS							(JUST A STUB FOR RIGHT NOW)
// Inputs: pointer to a void void process to add
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddProcess(void(*entry)(void), void *text, void *data, unsigned long stackSize, unsigned long priority){ 
	long status = StartCritical();
	
	// Find first PCB entry that isnt used
	int me = 0;
	while(pcbList[me].used){
		me++;
		if(me >= NUMPROC){		// Couldn't add process 
			Heap_Free(text);
			Heap_Free(data);
			EndCritical(status);
			return 0;
		}
	}

	pcbList[me].used = 1;
	pcbList[me].procID = ++globalPID;
	pcbList[me].entry = entry;
	pcbList[me].text = text;
	pcbList[me].data = data;
	pcbList[me].numThreads = 1;
	addedPID = pcbList[me].procID;
	
	OS_AddThread(entry, stackSize, priority);
  EndCritical(status);
  return 1;                                       // successful
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
	return runPt->ID;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime)
{
	// Unsure about actual multiplication timing/time step information
	runPt->sleep = sleepTime;
	OS_Suspend();
}

// ******** OS_Block ************
// place this thread into a blocked state
// input:  semaphore to block on
// output: none
int OS_Block(Sema4Type* semaPt)
{
	if (runPt->blocked) return 0;		// thread is already blocked
	// enter blocked status
	runPt->blocked = semaPt;		// add sema4Pt to tcb->blocked
	return 1;
}

// ******** OS_unBlock ************
// removed this thread from the blocked state
// input:  semaphore it is blocked on
// output: none
int OS_unBlock(Sema4Type* semaPt)
{
	// unblock highest pri thread
	int highest = 7;
	TCBtype* toUnblock = 0;
	int i;
	for (i = 0; i < NUMTHREADS; i++)		// cycle through all threads
	{
		// if blocked on this semaPt and is higher pri
		if (tcbList[i].blocked == semaPt && tcbList[i].priority < highest)
		{
			highest = tcbList[i].priority;
			toUnblock = &tcbList[i];
		}
	}
	if (toUnblock == 0) return 0;			// no thread to unblock
	// unblock by setting to 0
	toUnblock->blocked = 0;
	return 1;
}

// ******** OS_DecrementSleep ************
// Run every 1 ms by the OS Time timer
// Decrements all sleeps in tcbList
// input:  none
// output: none
void OS_DecrementSleep(void)
{
	int i;
	for(i = 0; i < NUMTHREADS; i++){
		if(tcbList[i].sleep > 0)
			tcbList[i].sleep--;
	}
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void)
{
	int i;
	for( i = 0; i < NUMPROC; i++){
		// Find process of thread and check if it is last in process
		if(runPt->PID == pcbList[i].procID){
			if(pcbList[i].numThreads == 1){
				// Kill the process
				// And clear up heap
				pcbList[i].numThreads = 0;
				pcbList[i].used = 0;
				Heap_Free(pcbList[i].text);
				Heap_Free(pcbList[i].data);
			}
			else{
				pcbList[i].numThreads--;
			}
		}
	}
	
	// Need to do updates here in case we remove the only TCB of that priority
	if(runPt->next == runPt){
		priM[runPt->priority] = 0;
	}
	
	// Remove thread from tcbList so it is not run again
	runPt->last->next = runPt->next;
	runPt->next->last = runPt->last;
	
	// Free tcb for use by new threads
	runPt->used = 0;
	
	// Immediately trigger context switch
	OS_Suspend();
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void)
{
	// Reset timer and set interrupt flag for context switch
	NVIC_ST_CURRENT_R = 0;
	NVIC_INT_CTRL_R = 0x04000000;
	//NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

///******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(uint32_t theTimeSlice){
	// Reset the systick for the start
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
	
	// Comment this out for cooperative
  NVIC_ST_CTRL_R = 0x00000007;         // enable, core clock and interrupt arm
	//runPt = &tcbList[0];                 // should be changed to find the first in use tcb
	// Immediately call scheduler to start up first thread
  Scheduler();
	// Start on the first task
	StartOS();
}


//******** Scheduler *************** 
//	A c function that allows us to determine what will be scheduled next
//  The systick handler calls this function in itself
void Scheduler(void)
{	
	int i;
	// cycle through all priorities
	for(i = 0; i < NUMPRIORITIES; i++){
		// if no threads of a priority
		if(priM[i] == 0)
			continue;
		
		// handle threads of priority i
		else{
			TCBtype *head = priM[i];
			TCBtype *start = head;
			
			// find non-sleeping/non-blocked thread of this priority
		  if (!head->sleep && !head->blocked)
			{
				// head was ready to run
				priM[i] = head->next;			// make sure next to run is first in line
				runPt = head;							// set head as currently running
				return;
			}
			
			// head was not ready to run
			while(head->sleep || head->blocked) {
				// cycle thru to fine next ready thread
				head = head->next;
				if (head == start)
				{
					// we made it back to the start
					// all threads are sleeping or blocked
					// move on to next priority level
					break;
				}
				else if (!head->sleep && !head->blocked)
				{
					// found a thread ready to run
					priM[i] = head->next;
					runPt = head;
					return;
				}
			}
		}
	}
}

//******** OS_DisTime *************** 
//	A c function that allows us to determine time disabled
unsigned long disTime = 0;
unsigned long disTimeMax = 0;
unsigned long disTimeTotal = 0;
void OS_DisTime(void)
{
	if(init)
		disTime = OS_Time();
}

//******** OS_EnTime *************** 
//	Another c function that allows us to determine time disabled
void OS_EnTime(void)
{
	if(init){
		unsigned long diff = OS_TimeDifference(disTime,OS_Time());
		if(diff > disTimeMax)
			disTimeMax = diff;
		disTimeTotal += diff;
	}
}

// ******** OS_MaxTimeDisabled ************
// reads the max time disabled
// Inputs:  none
// Outputs: time in us units
unsigned long OS_MaxTimeDisabled(void)
{
	return disTimeMax;				// returns in us resolution
}

// ******** OS_MaxTimePercent ************
// reads the max time disabled percentage
// Inputs:  none
// Outputs: time in us units
unsigned long OS_MaxTimePercent(void)
{
	return (disTimeMax * 100)/OS_Time();				// returns in us resolution
}

// ******** OS_MaxTimePercent ************
// Stores Time and ID of thread
// Inputs:  none
// Outputs: none
void ThreadProfiler(int16_t id)
{
	if(profile < 100){
		threadProfilerTime[profile] = OS_Time();
		threadProfilerId[profile++] = id;
	}
}

void* getProcessData(void)
{
	int i;
	for(i = 0; i < NUMPROC; i++){
		if(runPt->PID == pcbList[i].procID)
			return pcbList[i].data;
	}
	return 0;
}

// ******** HardFault_Handler ************
// Toggles HardFault signal
// Reports which thread hardfaulted
// Kills offending thread and continues operation
// Inputs:  none
// Outputs: none
#define PF1     (*((volatile uint32_t *)0x40025008))
void HardFault_Handler(void)
{
	while (1)
	{
		//PF1 = 0x02;
	}
}

