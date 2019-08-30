#include <stdint.h>
#include <stdio.h>
void OS_Init(void);

typedef struct TCBstruct TCB_t;

struct TCBstruct{
	uint32_t sp;
	uint32_t tcb_pointer;
	uint32_t pid;
	uint32_t sleep_state;
	uint32_t priority;
	uint32_t block_state;
};


unsigned long Count1;   // number of times thread1 loops 
unsigned long Count2;   // number of times thread2 loops 
unsigned long Count3;   // number of times thread3 loops 
 int NumCreated = 0 ;

void Thread1(void){   
Count1 = 0;             
for(;;){  
PB2 ^= 0x04;       //heartbeat     
Count1++;     
OS_Suspend();      //cooperative multitasking   
} 
} 
void Thread2(void){   
Count2 = 0;            
for(;;){    
PB3 ^= 0x08;       //heartbeat    
Count2++;   
OS_Suspend();      //cooperative multitasking 
  } 
} 

void Thread3(void){   
Count3 = 0;             
for(;;){     
PB4 ^= 0x10;      // heartbeat    
Count3++;    
OS_Suspend();     // cooperative multitasking  
	} 
} 

 int main1(void){  		// Testmain1  
 OS_Init();           // initialize, disable interrupts  
 PortB_Init();        // profile user threads  
 NumCreated = 0 ;   
 NumCreated += OS_AddThread(&Thread1,128,1);   
 NumCreated += OS_AddThread(&Thread2,128,2);   	
 NumCreated += OS_AddThread(&Thread3,128,3);    // Count1 Count2 Count3 should be equal or off by one at all times   
 OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here  
 return 0;            // this never executes 
 } 
 
 
 void OS_Init(void){
	 
 }
 
 