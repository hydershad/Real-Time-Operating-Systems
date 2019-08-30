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

void Bumper_Init(void){                          
  volatile uint32_t delay;                         
 
	SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port C
	 delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTC_DIR_R &= ~0xC0;    // (c) make PC6-7 in 
  GPIO_PORTC_AFSEL_R &= ~0xC0;  //     disable alt funct on PC6-7
  GPIO_PORTC_DEN_R |= 0xC0;     //     enable digital I/O on PC6-7   
  GPIO_PORTC_PCTL_R &= ~0xFF000000; // configure PC6-7 as GPIO
  GPIO_PORTC_AMSEL_R = 0;       //     disable analog functionality on PC6-7
  GPIO_PORTC_IS_R &= ~0xC0;     // (d) PC6-7 is edge-sensitive
  GPIO_PORTC_IBE_R &= ~0xC0;    //     PC6-7 is not both edges
  GPIO_PORTC_IEV_R |= 0xC0;    //     PC6-7 RISING edge event
  GPIO_PORTC_ICR_R = 0xC0;      // (e) clear PC6-7
  GPIO_PORTC_IM_R |= 0xC0;      // (f) arm interrupt on PC6-7 *** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF0FFFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x00000004;      // (h) enable interrupt 2 in NVIC (BIT 2)
//  EnableInterrupts();           // (i) Clears the I bit
}
void GPIOPortC_Handler(void){
  GPIO_PORTC_ICR_R = 0xC0;      // acknowledge PC6-7
	bumper = GPIO_PORTC_ICR_R & 0xC0;
}


void ping_init(void){
	  volatile uint32_t delay;                         

	    SYSCTL_RCGCGPIO_R |= 0x02;            // 2) activate port B
	 delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTB_DIR_R &= ~0x54;    // (c) make PB6, 4, 2 inputs (echo)
	  GPIO_PORTB_DIR_R |= ~0xA8;    // (c) make PB7, 5, 3 Output (Trigger) 
  GPIO_PORTB_AFSEL_R &= ~0xFC;  //     disable alt funct on PB7-2
  GPIO_PORTB_DEN_R |= 0xFC;     //     enable digital I/O on PB7-2   
  GPIO_PORTB_PCTL_R &= ~0xFFFFFF00; // configure PB7-2 as GPIO
  GPIO_PORTB_AMSEL_R = 0;       //     disable analog functionality on PB
  GPIO_PORTB_IS_R &= ~0x54;     // (d) PB6, 4, 2 is edge-sensitive
  GPIO_PORTB_IBE_R &= ~0x54;    //     PB6, 4, 2 is not both edges
  GPIO_PORTB_IEV_R |= 0x54;    //     PB6, 4, 2 RISING edge event
  GPIO_PORTB_ICR_R = 0x54;      // (e) clear PB6, 4, 2
  GPIO_PORTB_IM_R |= 0x54;      // (f) arm interrupt on PB6, 4, 2 *** No IME bit as mentioned in Book ***
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFF0FFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x00000002;      // (h) enable interrupt 1 in NVIC (BIT 1)
	
	
}



//ir sensor adc init and software trigger
/*void ADC_Init3210(void){ 
  
	volatile uint32_t delay;                         
                        
//  SYSCTL_RCGC0_R |= 0x00010000; // 1) activate ADC0 (legacy code)
  SYSCTL_RCGCADC_R |= 0x00000001; // 1) activate ADC0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; // 1) activate clock for Port E
  delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTE_DIR_R &= ~0x0F;      // 3) make PE0-3
  GPIO_PORTE_AFSEL_R |= 0x0F;     // 4) enable alternate function on PE0-3
  GPIO_PORTE_DEN_R &= ~0x0F;      // 5) disable digital I/O on PE0-3
                                  // 5a) configure PE4 as ?? (skip this line because PCTL is for digital only)
//  GPIO_PORTE_PCTL_R = GPIO_PORTE_PCTL_R&0xFF00FFFF;
  GPIO_PORTE_AMSEL_R |= 0x0F;     // 6) enable analog functionality on PE4 PE5

	
	//  ADC0_PC_R = 0x01;         // configure for 125K samples/sec
  ADC0_SSPRI_R = 0x3210;    // sequencer 0 is highest, sequencer 3 is lowest
  ADC0_ACTSS_R &= ~0x08;    // disable sample sequencer 3
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+0x5000; // timer trigger event
  ADC0_SSMUX3_R = 0X0123;				  // PE3 is pulled out first , PE0 last
  ADC0_SSCTL3_R = 0x06;          // set flag and end                       
  ADC0_IM_R |= 0x08;             // enable SS3 interrupts
  ADC0_ACTSS_R |= 0x08;          // enable sample sequencer 3
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00004000; //priority 2
  NVIC_EN0_R = 1<<17;              // enable interrupt 17 in NVIC
	
	long status = StartCritical();
	SYSCTL_RCGCTIMER_R |= 0x01;   // activate timer0 
  delay = SYSCTL_RCGCTIMER_R;   // allow time to finish activating
	TIMER0_CTL_R = 0x00000000;    // disable timer0A during setup
  //TIMER0_CTL_R |= 0x00000020;   // enable timer0A trigger to ADC
  TIMER0_CFG_R = 0x0;             // configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER0_TAPR_R = 0;            // prescale value for trigger
  TIMER0_TAILR_R = 8000000-1;    // start value for trigger
  TIMER0_IMR_R = 0x01;
	TIMER0_IMR_R = 0x00000000;    // disable all interrupts
	NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; //priority 2
  NVIC_EN0_R = 1<<19;              // enable interrupt 19 in NVIC
	
	
  TIMER0_CTL_R |= 0x1;   // enable timer0A 16-b, periodic, no interrupts
	EndCritical(status);
}

//------------ADC_In89------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: two 12-bit result of ADC conversions
// Samples ADC8 and ADC9 
// 125k max sampling
// software trigger, busy-wait sampling
// data returned by reference
// data[0] is ADC8 (PE5) 0 to 4095
// data[1] is ADC9 (PE4) 0 to 4095


void ADC_In89(void){
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS2
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  data[3] = ADC0_SSFIFO3_R&0xFFF;  // PE3
  data[2] = ADC0_SSFIFO3_R&0xFFF;  // PE2
  data[1] = ADC0_SSFIFO3_R&0xFFF;  // PE1
  data[0] = ADC0_SSFIFO3_R&0xFFF;  // PE0

	ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
}

void Timer0A_Handler(void){
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;
	ADC_In89();
}*/

/*int main (void){
	DisableInterrupts();
	UART_Init(3);
	ADC_Init3210();
		
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
                                        // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
	EnableInterrupts();
while(1){}	
	
}*/



/*void SysTick_Handler(void){
	
	ADC_In89();
	NLO();
	UART_OutUDec(data[3]);
	NLO();
	UART_OutUDec(data[2]);
	NLO();
	UART_OutUDec(data[1]);
	NLO();
	UART_OutUDec(data[0]);	
}*/


