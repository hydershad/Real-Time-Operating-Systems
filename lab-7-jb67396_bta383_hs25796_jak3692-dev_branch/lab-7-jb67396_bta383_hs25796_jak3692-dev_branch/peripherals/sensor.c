//sensor.c

//sensor board
//ir sensors on PD1-3
//bumper switches on PC6-7

//J9XY Trigger = PB7, Echo = PB6 
//J10XY Trigger = PB5, Echo = PB4 
//J11XY Trigger = PB3, Echo = PB2 
//J12XY Trigger = PC5, Echo = PF4 


//motor board
//PB7 = PWM motor A+
//PB6 = digital A-
//PB5 = PWM motor B+
//PB4 = digital motor B-

//PD0 = Servo A (steering) PWM
//PE4 = CAN Rx
//PE5 = CAN Tx

#include "tm4c123gh6pm.h"
#include "UART.h"
#include "can0.h"
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

//volatile int data [4] = {0};
int bumper = 0;
#define PF2     (*((volatile uint32_t *)0x40025010))
void Bumper_Init(void){                          
  volatile uint32_t delay;                         
 
	SYSCTL_RCGCGPIO_R |= 0x00000004; // (a) activate clock for port C
	delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
	GPIO_PORTC_AFSEL_R &= ~0xC0;  //     disable alt funct on PC6-7
  GPIO_PORTC_PCTL_R &= ~0xFF000000; // configure PC6-7 as GPIO
  GPIO_PORTC_AMSEL_R = 0;       //     disable analog functionality on PC6-7	
	GPIO_PORTC_DIR_R &= ~0xC0;    // (c) make PC6-7 in 
	GPIO_PORTC_DEN_R |= 0xC0;     //     enable digital I/O on PC6-7   
  
  GPIO_PORTC_IS_R &= ~0xC0;     // (d) PC6-7 is edge-sensitive
  GPIO_PORTC_IBE_R &= ~0xC0;    //     PC6-7 is not both edges
  GPIO_PORTC_IEV_R |= 0xC0;    //     PC6-7 RISING edge event
  GPIO_PORTC_ICR_R = 0xC0;      // (e) clear PC6-7
  GPIO_PORTC_IM_R |= 0xC0;      // (f) arm interrupt on PC6-7 *** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF0FFFFF)|0x00600000; // priority 3 changed from => 0x00A(g) priority 5
  NVIC_EN0_R = 0x00000004;      // (h) enable interrupt 2 in NVIC (BIT 2)
//  EnableInterrupts();           // (i) Clears the I bit
}
void GPIOPortC_Handler(void){
	uint8_t message[4];
	// Check front bumper
	if(GPIO_PORTC_RIS_R&0xC0){  // poll PC7
		PF2 ^= 0x04;
		uint32_t i;
			
		// Doing all of this in an interrupt context might be fine
		// or it might not
		message[3] = 1;                  // front bumper hit
		message[2] = 10; //Full reverse
		message[1] = 10; //Full reverse
		CAN0_SendData(message);
		
		//Wait some amount of time
		for(i = 0; i < 1000000; i++);
		
		//Check to see if it is hit.....
		
		
		
		message[3] = 1;                  // front bumper hit
		message[2] = 101; //Full reverse
		message[1] = 101; //Full reverse
		CAN0_SendData(message);
		
		for(i = 0; i < 4000000; i++);
		PF2 ^= 0x04;
		GPIO_PORTC_ICR_R = 0xC0;  // acknowledge flag7
		
  }
	/*else if (GPIO_PORTC_RIS_R&0x40){  // poll PC6
		PF2 ^= 0x04;
		// Doing all of this in an interrupt context might be fine
		// or it might not
		message[3] = 1;                  // left bumper hit
		message[2] = 100; //Full forward
		message[1] = 50;  //Half forward
		CAN0_SendData(message);
		
		//Wait some amount of time
		uint32_t i;
		for(i = 0; i < 4000000; i++);
		PF2 ^= 0x04;
		GPIO_PORTC_ICR_R = 0x40;  // acknowledge flag6
  }*/
	// Check other bumpers?
  /*if(GPIO_PORTC_RIS_R&0x20){  // poll PE5
    GPIO_PORTC_ICR_R = 0x20;  // acknowledge flag5
    SW2 = 1;                  // signal SW2 occurred
  }*/
  //GPIO_PORTC_ICR_R = 0xC0;      // acknowledge PC6-7
	//bumper = GPIO_PORTC_ICR_R & 0xC0;
}
