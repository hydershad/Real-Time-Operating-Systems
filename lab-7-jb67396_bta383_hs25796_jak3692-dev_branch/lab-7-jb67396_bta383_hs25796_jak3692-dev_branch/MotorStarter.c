// main.c
// Runs on TM4C123
// Real Time Operating System main launcher

#include "tm4c123gh6pm.h"
// Include all needed OS functions
#include "motorPWM.h"
#include "OS.h"
#include "can0.h"
#include "ADC.h"
#include "AperiodicTask.h"
#include "eDisk.h"
#include "eFile.h"
#include "FIFO.h"
#include "Interpreter.h"
#include "MailBox.h"
#include "PeriodicTask.h"
#include "PLL.h"
#include "Semaphore.h"
#include "ST7735.h"
#include "Time.h"
#include "UART.h"
#include "Heap.h"

void WaitForInterrupt(void);

//-------------- Test Code values -----------------------------
#define PERIOD TIME_500US   // Used for periodic threads

//-------------- End Test Code values -------------------------

/*
//--------------Debug Port Init-----------------------------
#define PE0  (*((volatile unsigned long *)0x40024004))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))
#define PE3  (*((volatile unsigned long *)0x40024020))
void PortE_Init(void)
{ 
  SYSCTL_RCGCGPIO_R |= 0x10;        // activate port E
  while((SYSCTL_RCGCGPIO_R&0x10)==0){};      
  GPIO_PORTE_DIR_R |= 0x0F;    			// make PE3-0 output heartbeats
  GPIO_PORTE_AFSEL_R &= ~0x0F;   		// disable alt funct on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     		// enable digital I/O on PE3-0
  GPIO_PORTE_PCTL_R = ~0x0000FFFF;
  GPIO_PORTE_AMSEL_R &= ~0x0F;     // disable analog functionality on PF
}

#define PF1     (*((volatile uint32_t *)0x40025008))
#define PF2     (*((volatile uint32_t *)0x40025010))
#define PF3     (*((volatile uint32_t *)0x40025020))
void PortF_Init(void)
{	
	// Init PF2 for profiling
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x20;  // activate port F
	delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
  GPIO_PORTF_DIR_R |= 0x0E;   // make PF2 output (PF2 built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x0E;// disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x0E;   // enable digital I/O on PF2
                              // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF000F)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
}
//-------------- end of Debug Port Init -----------------------------
*/



//------------------ IdleTask --------------------------------
// This runs an endless loop so there is always some thread
// running in the OS
void MotorIdleTask(void){ 
	// TODO if we add a suspend with no other threads running
	// will it just start running here again with no problem?
  for(;;)
	{ 
		//PE0 ^= 0x01; 
		WaitForInterrupt();
	}          // endless loop
}
//-------------- end of IdleTask -----------------------------

#define SERVORIGHT 655
#define SERVOMID 1855
#define SERVOLEFT 3125
#define SERVODELTA 100
// SW2 cycles through 12 positions
// mode=0 1875,...,1275
// mode=1 1870,...,2375
uint32_t Steering;     // 625 to 3125
uint32_t SteeringMode; // 0 for increase, 1 for decrease
#define POWERMIN 400
#define POWERMAX 12400
#define POWERDELTA 2000
uint32_t Power;
void DelayWait10ms(uint32_t n);

//------------------ CANMotorTask --------------------------------
// This simply gets the value from the CAN and sets the movement based on it 
void MotorCANTask(void){ 
	uint8_t message[4];
  for(;;)
	{ 
		//if(CAN0_CheckMail()){	
			//CAN0_GetMailNonBlock(message);
			// Do something based on message here 
			// ..................................
			CAN0_GetMail(message);
			if(message[3] == 1){					// Set motors
				if(message[2] <= 100 && message[1] <= 100){
					int diff = message[2] - message[1];
					if(diff < -65){
						int servo_set = SERVOMID - 100;
						if(diff < -75)
							servo_set += -150;
						if(diff < -85)
							servo_set += -300;		
						if(diff < -90)
							servo_set = SERVORIGHT;		
						Servo_Duty(servo_set);
						//Servo_Duty(SERVORIGHT);
						Right_Duty(12500-(((message[2]) * POWERMAX) / 100), 1);	// +5
						Left_DutyB(12500-((message[1] * POWERMAX) / 100), 1);
					}
					else if(diff > 65){
						int servo_set = SERVOMID + 100;
						if(diff > 75)
							servo_set += 250;
						if(diff > 80)
							servo_set += 300;	
						if(diff > 85)
							servo_set = SERVOLEFT;	
						Servo_Duty(servo_set);
						//Servo_Duty(SERVOLEFT);			
						Right_Duty(12500-((message[2] * POWERMAX) / 100), 1);	
						Left_DutyB(12500-(((message[1] + 10) * POWERMAX) / 100), 1); // +5 // +12
					}
					else{
						Servo_Duty(SERVOMID);
						Right_Duty(12500-((message[2] * POWERMAX) / 100), 1);	
						Left_DutyB(12500-((message[1] * POWERMAX) / 100), 1);
					}
					
				}
				else{
					Right_Duty(12500-(((message[2]-100) * POWERMAX) / 100), 0);	
					Left_DutyB(12500-(((message[1]-100) * POWERMAX) / 100), 0);
				}
				
				/*if(message[2] <= 100)
					Right_Duty(12500-((message[2] * POWERMAX) / 100), 1);	
				else
					Right_Duty(12500-(((message[2]-100) * POWERMAX) / 100), 0);	
				if(message[1] <= 100)
					Left_DutyB(12500-((message[1] * POWERMAX) / 100), 1);
				else
					Left_DutyB(12500-(((message[1]-100) * POWERMAX) / 100), 0);
				*/
			}			
			else if(message[3] == 2){			// Set servos
				if(message[0] == 0){
					Servo_Duty(SERVOLEFT);
				}
				else if(message[0] == 90){
					Servo_Duty(SERVOMID);
				}
				else if(message[0] == 180){	
					Servo_Duty(SERVORIGHT);
				}
				else{
					// Should never reach here
					Right_Duty(12500, 1);	
					Left_DutyB(12500, 1);
				}
			}
		//}
	}          // endless loop
}
//-------------- end of CANMotorTask -----------------------------
	
//-------------- realmain -----------------------------
int Motormain(void){ 		//MotorMain
	// Initialize hardware
	PLL_Init(Bus80MHz);
	//PortE_Init();
	//PortF_Init();
	//enum Direction forward = FORWARD;
	//enum Direction backward = FORWARD;
	Power = 0;
  Steering = SERVOMID;  // 20ms period 1.5ms pulse
  SteeringMode = 0;
  //DRV8848_RightInit(12500, Power, backward);          // initialize PWM0, 100 Hz
	Right_Init(12500, 12500-Power, 1);
  //DRV8848_LeftInit(12500, 12400-Power,forward);   // initialize PWM0, 100 Hz
	Left_InitB(12500, Power, 1);

  Servo_Init(25000, Steering);   
	
	CAN0_Open();
	
	// Initialize Operating System and File System
  OS_Init();
  OS_MailBox_Init();
  OS_Fifo_Init(256);
	
	// create initial foreground threads
  OS_AddThread(&Interpreter,128,2); 
  OS_AddThread(&MotorIdleTask,128,7);
	
	OS_AddThread(&MotorCANTask,128,6);
	
	// Launch the operating system
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
//--------------end of main-----------------------------


