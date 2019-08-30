// main.c
// Runs on TM4C123
// Real Time Operating System main launcher

#include "tm4c123gh6pm.h"
// Include all needed OS functions
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
#include "sensor.h"

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
*/
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
	
	GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
	
	
	
}
//-------------- end of Debug Port Init -----------------------------




//------------------ IdleTask --------------------------------
// This runs an endless loop so there is always some thread
// running in the OS
void IdleTask(void){ 
	// TODO if we add a suspend with no other threads running
	// will it just start running here again with no problem?
  for(;;)
	{ 
		//PE0 ^= 0x01; 
		//WaitForInterrupt();
		ADC_In89();
	}          // endless loop
}
//-------------- end of IdleTask -----------------------------

//------------------ File System Objects --------------------------------
FRESULT OS_MountFresult;
static FATFS OS_g_sFatFs;
//------------------ End --------------------------------

//------------------ CANTask --------------------------------
// This simply displays newest data to LCD
void CANTask(void){ 
	uint8_t message[8];
	message[4] = 0x20; // ' '
	message[5] = 0x3A; // ':'
	message[6] = 0x20; // ' '
	message[7] = 0;
	//int pass;
  for(;;)
	{ 
		//if(CAN0_CheckMail()){
		CAN0_GetMail(message);
		ST7735_Message(1, 0, (char *)message, 0);
		//}
	}          // endless loop
}
//-------------- end of CANTask -----------------------------


Sema4Type collisionCAN;
uint8_t collidingStop = 0;
void CollisionDetect(unsigned long value){
	if(value > 2041 && collidingStop == 0){
		OS_bSignal(&collisionCAN);
		collidingStop = 1;
	}
	else if(collidingStop == 1 && value <= 2041){
		OS_bSignal(&collisionCAN);
		collidingStop = 0;
	}
}

extern int motorSpeed;
//------------------ CollisionTask --------------------------------
// This simply stops the motors if we are about to collide
void CollisionTask(void){ 
	uint8_t message[4];
	//int pass;
  for(;;)
	{ 
		OS_bWait(&collisionCAN);
		if(collidingStop == 1){
			message[3] = 1;
			message[2] = 0;
			CAN0_SendData(message);
			ST7735_Message(1,0,"Sent stop message ", 0);
		}
		else{
			message[3] = 1;
			message[2] = motorSpeed;
			CAN0_SendData(message);
			ST7735_Message(1,0,"Sent go message ", motorSpeed);
		}
	}          // endless loop
}
//-------------- end of CollisionTask -----------------------------

//------------------ PIDTask --------------------------------
// Periodic task that grabs latest data collect and makes decision
// based on data. Sets motor values and returns
extern int data[4];
int32_t XStar = 0;
int32_t XPrime;
int32_t UI = 0;
int32_t E[4]; //550
#define Kp 1850 //950 //2450,2,0
#define Ki 2
#define Kd 2
#define DT 2
void PIDTask(void){ 
	uint8_t message[4];
	
	if(OS_MsTime() < 180000){
		// Collect data here 
		// dataCollect();
		
		// Estimators here (ie convertt data into XPrime)
		//XPrime = data[2] - data[1];
		// 4 to 1 before
		XPrime = (((7*data[3]) + (2*data[2]))/9) - (((2*data[1]) + (7*data[0]))/9);
		
		// PID Control here 
		E[3] = E[2];
		E[2] = E[1];
		E[1] = E[0];
		E[0] = XStar - XPrime;
		
		int32_t UP = Kp * E[0] / 100;
		
		UI = UI + (Ki * E[0] * DT) / 1000;
		if(UI > 2850) //2450
			UI = 2850;
		else if (UI < -2850)
			UI = -2850;
		
		int32_t UD = Kd * (E[0] + 3*E[1] - 3*E[2] - E[3])/(1000*6*DT);

		int32_t U = UP + UI + UD;
		if(U > 8500){ //4250
			U = 8500;
		}
		else if(U < -8500){
			U = -8500;
		}
		
		int setSpeed = 9500;
		if(U < 1500 && U > -1500)
			setSpeed = 12500;
		
		// Set 12500 message based on control value (ie decompose U into Left and Right)
		int32_t left =  100*(setSpeed - U)/12500;
		int32_t right = 100*(setSpeed + U)/12500;

		if(left > 100)
			left = 100;
		else if(left < 1)
			left = 1;
		if(right > 100)
			right = 100;
		else if(right < 8)
			right = 8;
		
		
		message[3] = 1;
		message[2] = left;//right;
		message[1] = right;//left;
		
		CAN0_SendData(message);
	}
	else if(OS_MsTime() >= 180000){
		message[3] = 1;
		message[2] = 0;//right;
		message[1] = 0;//left;
		CAN0_SendData(message);
		
	}
}
//-------------- end of PIDTask -----------------------------



//-------------- realmain -----------------------------
int main(void){ 		//OSmain
	// Initialize hardware
	PLL_Init(Bus80MHz);
	//PortE_Init();
	PortF_Init();
	//ADC_Seq3Collect(3, 10, &CollisionDetect);
	ADC_Init3210();
	
	Output_Init();
	ST7735_SplitScreen();
	
	UART_Init();
	CAN0_Open();
	Bumper_Init();
	
	// Initialize Operating System and File System
  OS_Init();
  OS_MailBox_Init();
  OS_Fifo_Init(256); 
	OS_InitSemaphore(&collisionCAN, 0);
	
	Heap_Init();
	if (Heap_Test() == 1) ST7735_Message(0, 0, "Heap Error", 1);
	OS_MountFresult = f_mount(&OS_g_sFatFs, "", 0);
  if(OS_MountFresult){
    ST7735_Message(0, 0, "f_mount error", 0);
    while(1){};
  }
	
	// create initial foreground threads
  OS_AddThread(&Interpreter,128,1); 
  OS_AddThread(&IdleTask,128,7);
	//OS_AddThread(&CANTask,128,6);
	//OS_AddThread(&CollisionTask,128,6);
	OS_AddPeriodicThread(&PIDTask, 10*TIME_1MS, 3);
	
	
	
	int startButton = GPIO_PORTF_DATA_R;
	while(startButton & 0x10){
		startButton = GPIO_PORTF_DATA_R;
	}
	
	PF1 = 0x02;
	// Launch the operating system
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}
//--------------end of main-----------------------------


