// Timer2.c
// Runs on LM4F120/TM4C123
// Use TIMER2 in 32-bit periodic mode to request interrupts at a periodic rate
// Daniel Valvano
// May 5, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
  Program 7.5, example 7.6

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
 #define NUMTHREADS 8
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "OS.h"

void (*PeriodicTask)(void) = 0;   // user function
void (*PeriodicTask2)(void);
void (*PeriodicTask3)(void);
void (*PeriodicTask4)(void) = 0;
extern uint32_t OSTimerCount;
int last = 0x11;
int periodicity = 0;
int first_Time, second_Time = 0;
// ***************** Timer2_Init ****************
// Activate Timer2 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
// Outputs: none
 void Timer2_Init(void(*task)(void), unsigned long period){
	 if(PeriodicTask == 0){
		 first_Time = (period/5000) + 1;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  PeriodicTask = task;          // user function
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x40000000; // 8) priority 2
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
	 }
	 else{
	TIMER2_CTL_R = 0x00000000;
	TIMER2_TAILR_R = 500;    // 4) reload value
  PeriodicTask4 = task;          // user function
	TIMER2_CTL_R = 0x00000001;
		 second_Time = (period / 5000) + 1;
	 }
}


void Timer2A_Handler(void){
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;
	if(PeriodicTask4 != 0){
	periodicity++;
	if(periodicity%first_Time == 0){
  TIMER2_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER2A timeout
  (*PeriodicTask)(); 
}	// execute user task
	if(periodicity%second_Time == 0){
		(*PeriodicTask4)();
		
	}
	if(periodicity == first_Time*second_Time){
		periodicity = 0;
	}
}
	else{
		(*PeriodicTask)();
	}
	
}

void Timer3_Init(unsigned long period){
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
	TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  //TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
}

static void GPIOArm(void){
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC  
}
void Switch_Init(void(*task)(void)){
  // **** general initialization ****
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  while((SYSCTL_PRGPIO_R & 0x00000020) == 0){};
		PeriodicTask2 = task;
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R |= 0x10;     //     PF4 is both edges
  GPIOArm();

  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
 }
static void GPIOArm2(void){
  GPIO_PORTF_ICR_R = 0x1;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x1;      // (f) arm interrupt on PF0 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC  
}
void Switch_Init2(void(*task)(void)){
  // **** general initialization ****
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  while((SYSCTL_PRGPIO_R & 0x00000020) == 0){};
	PeriodicTask3 = task;
	GPIO_PORTF_LOCK_R = 0x4C4F434B; // unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x11;  
  GPIO_PORTF_DIR_R &= ~0x1;    // (c) make PF0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x1;  //     disable alt funct on PF0
  GPIO_PORTF_DEN_R |= 0x1;     //     enable digital I/O on PF0   
  GPIO_PORTF_PCTL_R &= ~0x000F; // configure PF0 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x1;     //     enable weak pull-up on PF0
  GPIO_PORTF_IS_R &= ~0x1;     // (d) PF0 is edge-sensitive
  GPIO_PORTF_IBE_R |= 0x1;     //     PF0 is both edges
  GPIOArm2();

  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
 }
static void Timer3Arm(void){
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x0000001;    // 3) 1-SHOT mode
  TIMER3_TAILR_R = 160000;      // 4) 10ms reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN1_R = 1<<3;           // 9) enable IRQ 19 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER0A
}
void Timer3A_Handler(void){
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER3A timeout
	//OS_DisableInterrupts();
  	int temp = GPIO_PORTF_DATA_R;
	if(((last & 0x10) != 0)&&((temp&0x10) == 0)){
			(*PeriodicTask2)();
	}
	if(((last & 0x1) != 0)&&((temp&0x1) == 0)){
			(*PeriodicTask3)();
	}
	last = temp;
	if((temp&0x11)== 0x11){
		last = 0x11;
	}
	GPIOArm(); 
    // start GPIO
	GPIOArm2();
	//OS_EnableInterrupts();
}
void GPIOPortF_Handler(void){
	GPIO_PORTF_IM_R &= ~0x11;     // disarm interrupt on PF4 and PF0
  Timer3Arm(); // start one shot
} 


