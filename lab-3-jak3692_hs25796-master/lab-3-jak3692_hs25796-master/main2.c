//main2.c
#include <stdint.h>
#include <stdio.h>
extern unsigned long Count1;
extern unsigned long Count2;
extern unsigned long Count3;
void Thread1b(void){   
Count1 = 0;             
for(;;){     PB2 ^= 0x04;       // heartbeat     
Count1++;   
} 
} 

 
void Thread2b(void){ 
Count2 = 0;            
for(;;){   
PB3 ^= 0x08;       // heartbeat   
Count2++;  
} 
} 

void Thread3b(void){ 
Count3 = 0;        
for(;;){  
PB4 ^= 0x10;       // heartbeat  
Count3++;
} 
} 

int Testmain2(void){  // Testmain2 
OS_Init();           // initialize, disable interrupts  
PortB_Init();       // profile user threads  
NumCreated = 0 ;  
NumCreated += OS_AddThread(&Thread1b,128,1);   
NumCreated += OS_AddThread(&Thread2b,128,2);    
NumCreated += OS_AddThread(&Thread3b,128,3);    // Count1 Count2 Count3 should be equal on average  
// counts are larger than testmain1  
OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here  
return 0;            // this never executes 
} 

